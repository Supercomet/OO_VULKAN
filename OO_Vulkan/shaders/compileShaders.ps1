# Define the root directory to start searching
$rootDirectory = $PSScriptRoot 
Write-Host 
Write-Host "$($rootDirectory)"

Get-Location
Push-Location $($rootDirectory)
Set-Location $rootDirectory


$s = $Env:vulkan_sdk
Write-Host "Begin compile"
# Recursive function to process files based on their extensions in all subdirectories
function Process-FilesRecursively {
    param (
        [string]$path
    )

    # Get all files in the current directory
    $files = Get-ChildItem -Path $path -File

    # Process each file based on its extension
    foreach ($file in $files) {
        $fileExtension = $file.Extension.ToLower()
        switch ($fileExtension) {
            ".vert" {
                # Perform actions for .vert files
                Invoke-Expression -Command "$($s)\Bin\glslc.exe --target-env=vulkan1.3 -std=460 -O $($file.FullName) -o bin/$($file.Name).spv"
                # Add your specific actions here for .vert files
            }
            ".frag" {
                # Perform actions for .frag files
                Invoke-Expression -Command "$($s)\Bin\glslc.exe --target-env=vulkan1.3 -std=460 -O $($file.FullName) -o bin/$($file.Name).spv"
                # Add your specific actions here for .frag files
            }
            ".comp" {
                # Perform actions for .comp files
                Invoke-Expression -Command "$($s)\Bin\glslc.exe --target-env=vulkan1.3 -std=460 -O $($file.FullName) -o bin/$($file.Name).spv"
                # Add your specific actions here for .comp files
            }
            ".glsl" {
                # Perform actions for .glsl files
                # Invoke-Expression -Command "$($s)\Bin\glslc.exe --target-env=vulkan1.3 -std=460 -O $($file.FullName) -o bin/$($file.Name).spv"
                # Add your specific actions here for .glsl files
            }
            default {
                # Handle other file types here, if needed
                # Write-Host "Ignoring file with unknown extension: $($file.FullName)"
            }
        }
    }

    # Get all subdirectories in the current directory
    $subdirectories = Get-ChildItem -Path $path -Directory

    # Recursively call the function for each subdirectory
    foreach ($subdirectory in $subdirectories) {
        Process-FilesRecursively -path $subdirectory.FullName
    }
}

# Call the function with the root directory
Process-FilesRecursively -path $rootDirectory

Invoke-Expression -Command "$($s)\Bin\glslc.exe --target-env=vulkan1.3 -DFFX_GLSL -DFFX_GPU -fshader-stage=comp -O  fidelity\src\backends\vk\shaders\spd\ffx_spd_downsample_pass.glsl -o bin/$($file.Name).spv -I fidelity\include\FidelityFX\gpu\"
Write-Host "End Compile"

Pop-Location