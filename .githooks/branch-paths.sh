#!/bin/bash
# pre-commit hook (.pre-commit-config.yaml, id: branch-paths): voxlide/main = main + voxlide/.
#  - on main, no voxlide/ changes (main is the voxlide-free reference)
#  - on voxlide/* branches, commit changes to voxlide/ only; everything else is committed to main and merged in
# Merge commits (MERGE_HEAD present, e.g. after resolving conflicts) are not checked.
branch=$(git symbolic-ref --short -q HEAD) || exit 0
gitdir=$(git rev-parse --git-dir)
[ -f "$gitdir/MERGE_HEAD" ] && exit 0
staged=$(git diff --cached --name-only --no-renames)
case $branch in
  main) bad=$(grep '^voxlide/' <<<"$staged")
        msg="main is voxlide-free; commit these on voxlide/main" ;;
  voxlide/*) bad=$(grep -v '^voxlide/' <<<"$staged")
        msg="$branch takes changes to voxlide/ only; commit these on main, then: git switch $branch && git merge main" ;;
  *) exit 0 ;;
esac
[ -z "$bad" ] && exit 0
echo "$msg:" >&2
sed 's/^/  /' <<<"$bad" >&2
echo "(override once with: git commit --no-verify)" >&2
exit 1
