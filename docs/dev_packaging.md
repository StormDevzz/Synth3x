# Developer Guide: Custom Packaging (syn)

Synth3x includes a simple C-based package manager (`syn`) in `src/commands/syn.c`, alongside the full Portage/emerge capabilities.

---

## 1. How the `syn` Package Manager Works

`syn` operates with a local catalog database stored under `/var/db/syn/` and `/var/cache/syn/`:
*   `syn list`: Scans local package entries.
*   `syn search <query>`: Scans remote or cached package databases.
*   `syn inst <package>`: Fetches and extracts compiled tarball binaries.

---

## 2. Creating a Custom Package Recipe

To create a binary package:
1.  Compile your package on a target system using your desired optimization flags.
2.  Tar the files preserving prefix directories:
    ```bash
    tar -cvzf myapp-1.0.tar.gz usr/bin/myapp usr/share/applications/myapp.desktop
    ```
3.  Calculate the SHA256 checksum of the tarball.
4.  Add a recipe definition entry in your custom server's catalog index:
    ```json
    {
      "name": "myapp",
      "version": "1.0",
      "sha256": "abcdef...",
      "url": "https://mirrors.myserver.com/syn/myapp-1.0.tar.gz"
    }
    ```

---

## 3. Customizing Package Server Mirrors

The package mirrors url list is parsed in `src/commands/syn.c`. You can update it by changing the mirror database definition and recompiling `syn`:
```bash
make build/syn
```
