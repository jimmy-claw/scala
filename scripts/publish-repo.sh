#!/usr/bin/env bash
# publish-repo.sh — Sign .lgx packages, update repo index, create GitHub release
# Run after `nix build .#lgx-portable` and `cd ../scala-ui && nix build .#lgx-portable`
#
# Usage: bash scripts/publish-repo.sh [--dry-run]
#
# Requires: gh (authenticated), python3 with cryptography package, signing key at ~/.scala-repo-signing-key.pem

set -euo pipefail

DRY_RUN=false
if [[ "${1:-}" == "--dry-run" ]]; then
    DRY_RUN=true
    echo "=== DRY RUN MODE ==="
fi

REPO="jimmy-claw/scala"
SIGNING_KEY="${HOME}/.scala-repo-signing-key.pem"
WORK_DIR=$(mktemp -d)
trap 'rm -rf "$WORK_DIR"' EXIT

echo "Working dir: $WORK_DIR"

# ── 1. Collect .lgx files ────────────────────────────────────────────────
echo ""
echo "=== Collecting .lgx artifacts ==="

declare -a LGX_FILES=()
declare -A LGX_NAMES=()

# Find all .lgx files in result/ directories
for lgx in $(find . -path '*/result/*' -name '*.lgx' -o -name 'result' -type d 2>/dev/null); do
    if [[ -d "$lgx" ]]; then
        # result/ directory — find .lgx inside
        for f in "$lgx"/*.lgx; do
            [[ -f "$f" ]] && LGX_FILES+=("$f")
        done
    elif [[ -f "$lgx" ]]; then
        LGX_FILES+=("$lgx")
    fi
done

# Also check for .lgx in current directory (manual placement)
for f in *.lgx; do
    [[ -f "$f" ]] && LGX_FILES+=("$f")
done

if [[ ${#LGX_FILES[@]} -eq 0 ]]; then
    echo "ERROR: No .lgx files found. Run nix build first."
    exit 1
fi

echo "Found ${#LGX_FILES[@]} .lgx file(s):"
for f in "${LGX_FILES[@]}"; do
    name=$(basename "$f" .lgx)
    LGX_NAMES["$name"]="$f"
    echo "  $f ($(stat --format='%s' "$f") bytes)"
done

# ── 2. Sign and repack each .lgx ─────────────────────────────────────────
echo ""
echo "=== Signing packages ==="

python3 << PYEOF
import json, hashlib, base64, os, sys, tarfile, io, shutil
from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey
from cryptography.hazmat.primitives import serialization

work_dir = "$WORK_DIR"
signing_key = "$SIGNING_KEY"
dry_run = $DRY_RUN

# Load signing key
with open(signing_key, "rb") as f:
    priv = serialization.load_pem_private_key(f.read(), password=None)

# Build correct DID (base64url-encoded JWK)
pub_bytes = priv.public_key().public_bytes(
    serialization.Encoding.Raw, serialization.PublicFormat.Raw)
x_raw = base64.urlsafe_b64encode(pub_bytes).decode().rstrip('=')
jwk_obj = {"crv": "Ed25519", "kty": "OKP", "x": x_raw}
jwk_b64 = base64.urlsafe_b64encode(
    json.dumps(jwk_obj, separators=(',',':')).encode()
).decode().rstrip('=')
did = f'did:jwk:{jwk_b64}'

print(f"Signer DID: {did}")

# Process each .lgx
lgx_files = [
    $(printf '"%s"\n' "${LGX_FILES[@]}")
]

results = []  # Collect data for index update

for lgx_path in lgx_files:
    name = os.path.basename(lgx_path).replace('.lgx', '')
    print(f"\nProcessing: {name}")
    
    # Read original .lgx
    with open(lgx_path, "rb") as f:
        orig_data = f.read()
    
    # Extract to temp dir
    extract_dir = os.path.join(work_dir, name)
    os.makedirs(extract_dir, exist_ok=True)
    
    with tarfile.open(fileobj=io.BytesIO(orig_data), mode="r:gz") as tf:
        for member in tf.getmembers():
            # Use filter='data' equivalent for Python < 3.14
            if member.isfile():
                f = tf.extractfile(member)
                if f:
                    data = f.read()
                    out_path = os.path.join(extract_dir, member.name)
                    os.makedirs(os.path.dirname(out_path), exist_ok=True)
                    with open(out_path, "wb") as out:
                        out.write(data)
                    # Preserve mode
                    os.chmod(out_path, member.mode)
    
    # Read manifest.json bytes and generate signature
    manifest_path = os.path.join(extract_dir, "manifest.json")
    with open(manifest_path, "rb") as f:
        manifest_bytes = f.read()
    manifest = json.loads(manifest_bytes)
    
    # Sign the raw manifest bytes
    sig = base64.b64encode(priv.sign(manifest_bytes)).decode()
    
    # Create manifest.sig (same format as hackyguru's)
    sig_obj = {
        "algorithm": "ed25519",
        "did": did,
        "linkedDids": [],
        "signature": sig,
        "signer": {},
        "version": 1
    }
    
    sig_path = os.path.join(extract_dir, "manifest.sig")
    with open(sig_path, "w") as f:
        json.dump(sig_obj, f, indent=4)
    
    # Repack using subprocess tar (preserves checksums correctly)
    out_name = f"{name}-signed.lgx"
    out_path = os.path.join(work_dir, out_name)
    
    import subprocess
    result = subprocess.run(
        ["tar", "czf", out_path, "manifest.json", "manifest.sig", "variants"],
        cwd=extract_dir,
        capture_output=True, text=True
    )
    if result.returncode != 0:
        print(f"  TAR ERROR: {result.stderr}")
        sys.exit(1)
    
    # Compute new sha256 and size
    with open(out_path, "rb") as f:
        new_data = f.read()
    new_sha256 = hashlib.sha256(new_data).hexdigest()
    new_size = len(new_data)
    
    print(f"  Signed → {out_name} ({new_size} bytes)")
    print(f"  SHA256: {new_sha256}")
    print(f"  Version: {manifest['version']}")
    print(f"  Type: {manifest['type']}")
    
    results.append({
        "name": manifest["name"],
        "display_name": name,
        "output_file": out_path,
        "sha256": new_sha256,
        "size": new_size,
        "version": manifest["version"],
        "type": manifest["type"],
        "root_hash": manifest["hashes"]["root"],
        "manifest": manifest
    })

# Write results as JSON for the next step
with open(os.path.join(work_dir, "sign_results.json"), "w") as f:
    json.dump(results, f, indent=2)

print(f"\n=== Signed {len(results)} package(s) ===")
PYEOF

# ── 3. Create GitHub Release ─────────────────────────────────────────────
echo ""
echo "=== Creating GitHub Release ==="

# Read version from first package
VERSION=$(python3 -c "import json; data=json.load(open('$WORK_DIR/sign_results.json')); print(data[0]['version'])")
RELEASE_TAG="v${VERSION}"

# Collect signed .lgx files for upload
SIGNED_FILES=$(python3 -c "import json; data=json.load(open('$WORK_DIR/sign_results.json')); [print(r['output_file']) for r in data]")

if [[ "$DRY_RUN" == "false" ]]; then
    # Delete existing release with same tag if it exists
    gh release delete "$RELEASE_TAG" --yes 2>/dev/null || true
    
    # Create new release
    gh release create "$RELEASE_TAG" \
        $SIGNED_FILES \
        --title "Scala $VERSION" \
        --notes "Auto-published from CI — signed packages for basecamp module repository" \
        2>&1 | head -3
    
    echo "Release created: https://github.com/$REPO/releases/tag/$RELEASE_TAG"
else
    echo "[dry-run] Would create release $RELEASE_TAG with:"
    for f in $SIGNED_FILES; do
        echo "  $(basename "$f")"
    done
fi

# ── 4. Update repo-index.json ────────────────────────────────────────────
echo ""
echo "=== Updating repo-index.json ==="

python3 << PYEOF
import json, base64, os
from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey
from cryptography.hazmat.primitives import serialization

work_dir = "$WORK_DIR"
signing_key = "$SIGNING_KEY"
repo = "$REPO"
dry_run = $DRY_RUN
version = "$VERSION"

# Load signing key
with open(signing_key, "rb") as f:
    priv = serialization.load_pem_private_key(f.read(), password=None)

pub_bytes = priv.public_key().public_bytes(
    serialization.Encoding.Raw, serialization.PublicFormat.Raw)
x_raw = base64.urlsafe_b64encode(pub_bytes).decode().rstrip('=')
jwk_obj = {"crv": "Ed25519", "kty": "OKP", "x": x_raw}
jwk_b64 = base64.urlsafe_b64encode(
    json.dumps(jwk_obj, separators=(',',':')).encode()
).decode().rstrip('=')
did = f'did:jwk:{jwk_b64}'

# Read sign results
with open(os.path.join(work_dir, "sign_results.json")) as f:
    results = json.load(f)

# Build new index
import datetime
now = datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")

packages = []
for r in results:
    # Sign the sha256 for catalog entry
    sig = base64.b64encode(priv.sign(r["sha256"].encode())).decode()
    
    # Build release download URL
    filename = os.path.basename(r["output_file"])
    url = f"https://github.com/{repo}/releases/download/v{r['version']}/{filename}"
    
    pkg = {
        "name": r["name"],
        "versions": [{
            "releasedAt": now,
            "publisherRef": f"{r['name']}-v{r['version']}",
            "url": url,
            "size": r["size"],
            "sha256": r["sha256"],
            "rootHash": r["root_hash"],
            "manifest": r["manifest"],
            "signature": {
                "algorithm": "ed25519",
                "did": did,
                "linkedDids": [],
                "signature": sig,
                "signer": {},
                "version": 1
            }
        }]
    }
    packages.append(pkg)
    print(f"  {r['name']} v{r['version']} → {url[:60]}...")

index = {
    "schemaVersion": 2,
    "repositoryName": "jimmy-scala-modules",
    "generatedAt": now,
    "packages": packages
}

# Also update logos-repo.json
repo_json = {
    "schemaVersion": 1,
    "name": "jimmy-scala-modules",
    "displayName": "Jimmy's Scala Modules",
    "description": "SCALA — Secure Calendar App on Logos Core. Privacy-first P2P shared calendar.",
    "homepage": f"https://github.com/{repo}",
    "indexUrl": f"https://raw.githubusercontent.com/{repo}/main/repo-index.json",
    "trustedSigners": [{
        "did": did,
        "name": "jimmy-claw"
    }]
}

# Write files
with open("repo-index.json", "w") as f:
    json.dump(index, f, indent=2)
with open("logos-repo.json", "w") as f:
    json.dump(repo_json, f, indent=2)

print(f"\nUpdated repo-index.json + logos-repo.json")
print(f"Signer DID: {did}")
PYEOF

echo ""
if [[ "$DRY_RUN" == "false" ]]; then
    echo "=== Committing ==="
    git add repo-index.json logos-repo.json
    git commit -m "release: auto-publish v${VERSION} — signed packages + updated index" || echo "(no changes to commit)"
    echo ""
    echo "Ready to push: git push origin main"
else
    echo "[dry-run] Would update repo-index.json and logos-repo.json"
fi

echo ""
echo "=== Done ==="
