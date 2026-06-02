#!/usr/bin/env bash
# Push the local BlackDex tree to your GitHub repository.
#
# Usage:
#   GITHUB_TOKEN=ghp_xxx ./scripts/push_to_github.sh owner/repo [branch]
#
# The script will:
#   1. initialise a git repo (if needed)
#   2. commit the current working copy
#   3. push to the supplied repo + branch using the token
#
# You can also run the steps manually - this is just a convenience wrapper.
set -euo pipefail

REPO="${1:?Usage: $0 owner/repo [branch]}"
BRANCH="${2:-main}"
WORKDIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$WORKDIR"

if [[ -z "${GITHUB_TOKEN:-}" ]]; then
    echo "GITHUB_TOKEN env var is required." >&2
    exit 1
fi

if [[ ! -d .git ]]; then
    git init -q
    git config user.name  "blackbox-builder"
    git config user.email "blackbox-builder@localhost"
fi

git add -A
if git diff --cached --quiet; then
    echo "Nothing to commit."
else
    git commit -q -m "Add runtime artifact hider (anti-detection) and settings toggle"
fi

REMOTE_URL="https://x-access-token:${GITHUB_TOKEN}@github.com/${REPO}.git"
if git remote get-url origin >/dev/null 2>&1; then
    git remote set-url origin "$REMOTE_URL"
else
    git remote add origin "$REMOTE_URL"
fi

git push -u origin HEAD:"$BRANCH"
echo
echo "Pushed to https://github.com/${REPO} (branch: $BRANCH)"
