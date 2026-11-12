# VNKey
## Build

### 1. **Open Compiler Settings:**
   * Open **Code::Blocks**.
   * On the top menu bar, click on **Settings** -> **Compiler...**
### 2. **Navigate to Linker Settings:**
   * In the *Compiler settings* window that appears, click on the **Linker settings** tab at the top.

### 3. **Add the Linker Options:**
   * Look for the box labeled **Other linker options** (usually located on the right side).
   * Paste the following flags into the box:
   ```
   -static -static-libgcc -static-libstdc++
   ```

### 4. **Save Your Settings:**
   * Click the **OK** button at the bottom right to save the changes.

### 5. **Build Your Project:**
   * To apply the new configuration, go to the top menu and select **Build and run** (or press **F9** to build a new executable file).

## Adding an Icon to VNKey Application in Code::Blocks

Follow these steps to embed a custom icon into your Windows desktop application built with Code::Blocks and MinGW:

### 1. Prepare the Icon File
* Save your `.ico` file (e.g., `vnkey.ico`) directly inside your project's root directory (the same folder where your source code file is located).

### 2. Create a Resource File (.rc)
* In Code::Blocks, go to **File** -> **New** -> **Empty file**.
* When prompted to add it to the project, click **Yes**.
* Save the file as `resource.rc` in the project folder and make sure it is assigned to both **Debug** and **Release** build targets.

### 3. Add the Icon Script
* Open the `resource.rc` file and enter the following line:

  ```
  101 ICON "vnkey.ico"
  ```

### 4. Link Files to the Project
If the files are not yet added to a project, right-click your project name in the Management panel (left sidebar), select Add files..., and select both your .cpp code file and resource.rc.

Ensure both Debug and Release targets are checked when prompted.

### 5. Rebuild the Application
Press Ctrl + F11 (or go to Build -> Rebuild) to compile. MinGW will automatically execute windres to embed the icon into the final .exe file.

## Run

Open VNKey.exe