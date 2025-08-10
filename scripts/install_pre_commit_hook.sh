#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
HOOK_FILE="$ROOT_DIR/.git/hooks/pre-commit"
SCRIPT_REL="scripts/pre_commit_run.sh"

if [[ ! -d "$ROOT_DIR/.git" ]]; then
  echo "[pre-commit-install] Not a git repository: $ROOT_DIR" >&2
  exit 1
fi

cat >"$HOOK_FILE" <<EOF
#!/usr/bin/env bash
# Auto-generated Huntmaster pre-commit hook
exec bash "$SCRIPT_REL" "$@"
EOF
chmod +x "$HOOK_FILE"
echo "Installed pre-commit hook -> $HOOK_FILE"
