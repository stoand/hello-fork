{
  "targets": [
    {
      "target_name": "addon",
      "sources": [ "src/addon.cpp" ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")"
      ],
      "defines": [ "NAPI_CPP_EXCEPTIONS" ],
      "cflags_cc": [
        "-std=c++17" # Or whatever C++ standard you prefer
      ],
      "conditions": [
        ['OS=="mac"', {
          'xcode_settings': {
            'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
          },
        }],
        ['OS=="linux"', {
          'cflags_cc': [
            "-fexceptions" # Enable exceptions for Linux
          ]
        }]
      ]
    }
  ]
}
