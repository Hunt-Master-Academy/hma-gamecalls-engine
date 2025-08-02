#!/bin/bash
# VS Code Issue Resolution Script
# Fixes event listener leaks and performance issues

set -e

echo "🔧 VS Code Issue Resolution Script"
echo "======================================"

# 1. Clear VS Code workspace state
echo "1. Clearing VS Code workspace state..."
if [ -d ".vscode" ]; then
    # Backup current settings
    cp -r .vscode .vscode.backup.$(date +%Y%m%d_%H%M%S) 2>/dev/null || true
    
    # Clear problematic cache files
    rm -f .vscode/*.log 2>/dev/null || true
    rm -rf .vscode/extensions 2>/dev/null || true
    rm -f .vscode/settings.json.bak* 2>/dev/null || true
fi

# 2. Optimize VS Code settings for large projects
echo "2. Optimizing VS Code settings..."
cat > .vscode/settings.json << 'EOF'
{
  "files.watcherExclude": {
    "**/build/**": true,
    "**/build-wasm/**": true,
    "**/coverage_analysis/**": true,
    "**/coverage_reports/**": true,
    "**/test_logs/**": true,
    "**/.cache/**": true,
    "**/.ccache/**": true,
    "**/node_modules/**": true,
    "**/*.gcda": true,
    "**/*.gcno": true,
    "**/*.gcov": true
  },
  "search.exclude": {
    "**/build/**": true,
    "**/build-wasm/**": true,
    "**/coverage_analysis/**": true,
    "**/coverage_reports/**": true,
    "**/test_logs/**": true,
    "**/.cache/**": true,
    "**/.ccache/**": true
  },
  "files.associations": {
    "*.cpp": "cpp",
    "*.h": "cpp",
    "*.hpp": "cpp"
  },
  "C_Cpp.intelliSenseEngine": "Default",
  "C_Cpp.maxCachedProcesses": 2,
  "C_Cpp.maxConcurrentThreads": 2,
  "C_Cpp.enhancedColorization": "Disabled",
  "editor.semanticHighlighting.enabled": false,
  "typescript.preferences.includePackageJsonAutoImports": "off",
  "typescript.suggest.autoImports": false,
  "javascript.suggest.autoImports": false,
  "files.trimTrailingWhitespace": true,
  "files.insertFinalNewline": true,
  "editor.formatOnSave": true,
  "workbench.settings.enableNaturalLanguageSearch": false
}
EOF

# 3. Reduce file monitoring load
echo "3. Creating .vscode/launch.json with optimized debug settings..."
cat > .vscode/launch.json << 'EOF'
{
  "version": "0.2.0",
  "configurations": [
    {
      "name": "Debug Tests",
      "type": "cppdbg",
      "request": "launch",
      "program": "${workspaceFolder}/build/bin/RunEngineTests",
      "args": ["--gtest_brief=yes"],
      "stopAtEntry": false,
      "cwd": "${workspaceFolder}",
      "environment": [],
      "externalConsole": false,
      "MIMode": "gdb",
      "setupCommands": [
        {
          "description": "Enable pretty-printing for gdb",
          "text": "-enable-pretty-printing",
          "ignoreFailures": true
        }
      ],
      "preLaunchTask": "build",
      "miDebuggerPath": "/usr/bin/gdb"
    }
  ]
}
EOF

# 4. Optimize tasks.json
echo "4. Optimizing tasks.json..."
cat > .vscode/tasks.json << 'EOF'
{
  "version": "2.0.0",
  "tasks": [
    {
      "label": "build",
      "type": "shell",
      "command": "ninja",
      "args": ["-C", "build"],
      "group": {
        "kind": "build",
        "isDefault": true
      },
      "presentation": {
        "echo": true,
        "reveal": "silent",
        "focus": false,
        "panel": "shared",
        "showReuseMessage": true,
        "clear": false
      },
      "problemMatcher": ["$gcc"]
    },
    {
      "label": "test",
      "type": "shell",
      "command": "timeout",
      "args": ["60", "./build/bin/RunEngineTests", "--gtest_brief=yes"],
      "group": "test",
      "presentation": {
        "echo": true,
        "reveal": "always",
        "focus": false,
        "panel": "shared",
        "showReuseMessage": true,
        "clear": true
      },
      "dependsOn": "build"
    }
  ]
}
EOF

# 5. Clear problematic caches
echo "5. Clearing caches and temporary files..."
rm -rf .vscode-server/data/User/workspaceStorage/* 2>/dev/null || true
rm -rf .cache/* 2>/dev/null || true
find . -name "*.tmp" -delete 2>/dev/null || true
find . -name "*.log" -path "*/test_logs/*" -delete 2>/dev/null || true

# 6. Optimize git for large repos
echo "6. Optimizing git configuration..."
git config core.preloadindex true 2>/dev/null || true
git config core.fscache true 2>/dev/null || true
git config gc.auto 256 2>/dev/null || true

echo "✅ VS Code optimization complete!"
echo ""
echo "📋 Recommended Next Steps:"
echo "1. Restart VS Code completely (close and reopen)"
echo "2. Wait for C++ extension to re-index (may take 2-3 minutes)"
echo "3. Monitor console for reduced error messages"
echo "4. If issues persist, consider running: code --disable-extensions"
echo ""
