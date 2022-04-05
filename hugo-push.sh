#!/bin/bash

###############################################################################
# 仓库地址：https://github.com/whowin/whowin.github.io.git
# token: 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
# git remote add origin https://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@github.com/whowin/whowin.github.io.git
#
# 在github上建立两个分支
# master分支：用于存放public目录下的内容，也就是hugo生成的网站的内容
# hugo分支：用于存放项目中除了public目录以外的内容，也就是网站的源码
#
# 本地仓库中加入.gitignore文件，将public目录排除在外
#
# hugo在生成静态网站的时候并不删除上次生成的内容，所以可能会有重叠
# 为避免这种重叠，在重建网站之前应该先删除原来的内容
###############################################################################
if [[ $(git status -s) ]]
then
    echo "The working directory is dirty. Please commit any pending changes."
    exit 1;
fi

echo "Deleting old publication"
rm -rf public
mkdir public
rm -rf .git/worktrees/public/

echo "Checking out hugo branch into public"
git worktree add -B hugo public origin/hugo

echo "Removing existing files"
rm -rf public/*

echo "Generating site"
hugo

echo "Updating hugo branch"
cd public && git add --all && git commit -m "Publishing to hugo (publish.sh)"

echo "Push to origin"
git push origin hugo

