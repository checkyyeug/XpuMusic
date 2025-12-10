# 批量重命名脚本：将 foobar2000 相关名称替换为 XpuMusic

# 需要替换的模式和对应的替换内容
$replacements = @{
    'foobar2000' = 'XpuMusic';
    'FoobarPluginLoader' = 'XpuMusicPluginLoader';
    'foobar_compatibility' = 'xpumusic_compatibility';
    'foobar_sdk' = 'xpumusic_sdk';
    'foobar2000_sdk' = 'xpumusic_sdk';
    'foobar2000::' = 'xpumusic::';
    'foobar2000_' = 'xpumusic_';
    'Foobar2000' = 'XpuMusic';
}

# 获取所有需要处理的文件
$files = Get-ChildItem -Path . -Recurse -File | Where-Object {
    $_.FullName -notlike "*\build\*" -and
    $_.FullName -notlike "*\build_*\*" -and
    ($_.Extension -eq ".cpp" -or $_.Extension -eq ".h" -or $_.Extension -eq ".md" -or
     $_.Extension -eq ".txt" -or $_.Name -eq "CMakeLists.txt" -or $_.Extension -eq ".cmake")
}

Write-Host "开始处理文件..."

foreach ($file in $files) {
    $content = Get-Content -Path $file.FullName -Raw
    $originalContent = $content

    # 应用所有替换规则
    foreach ($pattern in $replacements.Keys) {
        $content = $content -replace $pattern, $replacements[$pattern]
    }

    # 只有内容发生变化时才写入文件
    if ($content -ne $originalContent) {
        Set-Content -Path $file.FullName -Value $content -NoNewline
        Write-Host "已更新: $($file.FullName)"
    }
}

Write-Host "完成！"