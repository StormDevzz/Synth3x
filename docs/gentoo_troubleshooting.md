# Troubleshooting Gentoo Compilation & Dependencies

Compilation processes are prone to errors due to hardware constraints, package updates, or configuration issues. Below are standard debugging techniques.

---

## 1. Inspecting Portage Build Logs

When a compilation fails, Portage creates a detailed build log:
*   **Locating log:** `/var/tmp/portage/<category>/<package-name>-<version>/temp/build.log`
*   **Inspecting:** Scroll to the bottom of the log file and search backwards for errors or warnings (e.g. `gcc: error`, `undefined reference`, etc.).

---

## 2. Resolving Dependency Conflicts

### Problem: Package Masked
Gentoo masks packages that are experimental, unstable, or deprecated.
*   **Solution:** To unmask a testing package (e.g. `~amd64`), add it to `/etc/portage/package.accept_keywords`:
    ```bash
    echo "category/package-name ~amd64" >> /etc/portage/package.accept_keywords/custom
    ```

### Problem: Blocked Packages
Two packages require the same file resources or conflict.
*   **Solution:** Uninstall the blocking package first using `emerge --unmerge <blocker>` or check if you can change USE flags to resolve the overlap.

### Problem: Out of Memory (OOM) compiler crash
Compiling large packages (like Firefox or GCC) can exhaust system RAM.
*   **Solution:** Increase swap space, or limit parallel compiler execution using `MAKEOPTS="-j1"` temporarily for the heavy package.
