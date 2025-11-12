{
  "targets": [
    {
      "target_name": "terminfo",
      "sources": [ "src/native/addon.cc", "src/native/terminfo.c" ],
      "include_dirs": [
        # Prefer pkg-config cflags (quiet if not installed)
        "<!@(pkg-config --cflags --silence-errors ncursesw || pkg-config --cflags --silence-errors ncurses || echo)"
      ],
      "output_dir": "<!(node -e \"require('path').join(__dirname, 'src', 'native', 'build', 'Release')\")",
      "libraries": [
        # Linux: prefer wide ncurses (regular fallback), include tinfo when split
        "<!@(pkg-config --libs --silence-errors ncursesw || pkg-config --libs --silence-errors ncurses || echo -lncursesw -ltinfo)"
      ],
      "conditions": [
        [ "OS==\"linux\" or OS==\"mac\"", {
          "libraries": [ "-lncurses", "-ltinfo" ],
          "include_dirs": [ "-I/usr/local/share/nvm/versions/node/v25.1.0/include/node" ]
        } ]
      ],
      "variables": {
        "node_engine": "napi",
        "node_version": "25.1.0",
        "enable_lto": "true",
        "enable_pgo_generate": "false",
        "enable_pgo_use": "false",
        "output_dir": "<!(node -e \"require('path').join(__dirname, 'src', 'native', 'build', 'Release')\")"
      }
    }
  ]
}
