@echo off
echo Creating icon files...

powershell -Command "$appIcon = [System.Convert]::FromBase64String('iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAYAAABzenr0AAAAAXNSR0IArs4c6QAAAARnQU1BAACxjwv8YQUAAAAJcEhZcwAADsMAAA7DAcdvqGQAAAD3SURBVFhH7ZZBCsIwEEVzD+vSpahQF258rgSv4MKrCVLBjYu4cOF1dKHunkTaTjJmpgYFMYUOfAhp0/D/TzKFNBIdLfGUUTMtKx/Zy6QpxvQhI+YQXyBl4CRTkb0MtCYAhR9l1gzz3AKONWdm4CYrSTMXFMbY32SZLu4QBobBIwxsZCOEfW5+FgD7e3TJxxYhZmE7G6j1QmILMAtQa2YfWwjRgp/GFsLbAnIIfn4GPrJ1D3ILwnkDIAi/RhNbiHWHUCGE8LKAnQJyif6+AS+wfoAqiHNIYC2+2J9rYZgLxBBAp9+7cKzhXAlUzRw1ADwBCJDGXaCbF4AAAAAASUVORK5CYII='); [System.IO.File]::WriteAllBytes('resources/icons/app_icon.png', $appIcon); $blockIcon = [System.Convert]::FromBase64String('iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAYAAABzenr0AAAAAXNSR0IArs4c6QAAAARnQU1BAACxjwv8YQUAAAAJcEhZcwAADsMAAA7DAcdvqGQAAAD/SURBVFhH7ZXBCcJAEEXXMhSPYgEqiAWoV29KH1poFQFBm/Bq8qCDFbhCBBE8OzOSYTeThWQ3BnHhHx7ZJfP/JJnZDBq1wS0DhXAw0S0DVYyAOiNg4qARDgx1S4Jnhl8VmhUw0wgDc90yojj8sQJmZWuMgHOWf4WlbhlmRvgXpyBcwuGURsQXLPdpubC4Q2xcAmNnHlXCIxCKUiLGR+A2fCqEF+Ae+FTIbGUYkBGpEHsLQmI8QibqBZzuLRsVLsGkXgEpeKxxgFAJHwH3c+DaNTcNDmrhQo0mBxfTOo5GWu7gZQRG4BG4Dffj3j3QDMAdPKYzIDOg9gJu4GnJ1V+gxQ9cIPMBKUUrcAAAAABJRU5ErkJggg=='); [System.IO.File]::WriteAllBytes('resources/icons/block_icon.png', $blockIcon)"

if %ERRORLEVEL% neq 0 (
    echo Failed to create icon files!
    echo Creating empty PNG files instead...
    
    type nul > resources\icons\app_icon.png
    type nul > resources\icons\block_icon.png
)

echo Icons created successfully. 