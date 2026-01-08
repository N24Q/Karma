import os
import re
from pathlib import Path

# Path to textures directory
textures_dir = Path(r"c:\Dev\Projects\Karma\assets\models\basementroom9-cTextures\textures")

print(f"Scanning: {textures_dir}")
print("-" * 60)

renamed_count = 0
for file_path in textures_dir.glob("*"):
    if file_path.is_file():
        old_name = file_path.name
        # Remove hash suffix pattern like "-uZJp0djs" before file extension
        new_name = re.sub(r'-[a-zA-Z0-9]+\.', '.', old_name)

        if new_name != old_name:
            new_path = file_path.parent / new_name
            print(f"Rename: {old_name:60s} -> {new_name}")

            # Actually rename the file
            file_path.rename(new_path)
            renamed_count += 1

print("-" * 60)
print(f"Renamed {renamed_count} files successfully!")
