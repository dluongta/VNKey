## VNKey
### Set up

1. **Open Compiler Settings:**
   * Open **Code::Blocks**.
   * On the top menu bar, click on **Settings** -> **Compiler...**

2. **Navigate to Linker Settings:**
   * In the *Compiler settings* window that appears, click on the **Linker settings** tab at the top.

3. **Add the Linker Options:**
   * Look for the box labeled **Other linker options** (usually located on the right side).
   * Paste the following flags into the box:
    ```
    -static
    -static-libgcc
    -static-libstdc++
    ```

4. **Save Your Settings:**
   * Click the **OK** button at the bottom right to save the changes.

5. **Build Your Project:**
   * To apply the new configuration, go to the top menu and select **Build and run** (or press **F9** to build a new executable file).


### Run

Open VNKey.exe