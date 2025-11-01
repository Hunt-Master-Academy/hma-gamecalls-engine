# [20251028-BINDINGS-001] Node-API binding configuration for GameCalls Engine
# Multi-stage build: C++ engine → Node-API wrapper → JavaScript module
{
  'targets': [
    {
      'target_name': 'gamecalls_engine',
      'sources': [
        'src/gamecalls_addon.cc',
        'src/session_wrapper.cc',
        'src/audio_processor.cc',
        'src/type_converters.cc'
      ],
      'include_dirs': [
        "<!@(node -p \"require('node-addon-api').include\")",
        "../../include",
        "../../libs/kiss_fft",
        "../../libs/miniaudio"
      ],
      'dependencies': [
        "<!(node -p \"require('node-addon-api').gyp\")"
      ],
      'defines': [
        'NAPI_VERSION=8',
        'NAPI_DISABLE_CPP_EXCEPTIONS',
        'DISABLE_LOGGING=1'
      ],
      'cflags!': [ '-fno-exceptions' ],
      'cflags_cc!': [ '-fno-exceptions' ],
      'cflags_cc': [
        '-std=c++20',
        '-O3',
        '-Wall',
        '-Wextra'
      ],
      'conditions': [
        ['OS=="linux"', {
          'libraries': [
            '-L<(module_root_dir)/../../build/src',
            '-lUnifiedAudioEngineNoDiag',
            '-lasound',
            '-lpthread',
            '-lkissfft-float'
          ],
          'ldflags': [
            '-Wl,-rpath,<(module_root_dir)/../../build/src',
            '-L<(module_root_dir)/../../build/_deps/kissfft-build'
          ]
        }],
        ['OS=="mac"', {
          'libraries': [
            '-L<(module_root_dir)/../../build/src',
            '-lUnifiedAudioEngineNoDiag'
          ],
          'xcode_settings': {
            'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
            'CLANG_CXX_LIBRARY': 'libc++',
            'MACOSX_DEPLOYMENT_TARGET': '10.15',
            'OTHER_CPLUSPLUSFLAGS': [
              '-std=c++20',
              '-stdlib=libc++'
            ]
          }
        }],
        ['OS=="win"', {
          'libraries': [
            '-l<(module_root_dir)/../../build/src/Release/UnifiedAudioEngineNoDiag.lib'
          ],
          'msvs_settings': {
            'VCCLCompilerTool': {
              'ExceptionHandling': 1,
              'AdditionalOptions': ['/std:c++20']
            }
          }
        }]
      ]
    }
  ]
}
