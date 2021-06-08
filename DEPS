
vars = {

}

deps = {
  'tools':
    'https://chromium.googlesource.com/chromium/src/tools@d62ac9b1dbfa6447d042513d738baabfb4bccf6d',
  'build':
    'https://chromium.googlesource.com/chromium/src/build@833c1f757f772e19c858e8d327b51de052ea0f12',
  'buildtools':
    'https://chromium.googlesource.com/chromium/src/buildtools@4401ea90ed6aefafb78fc3907df1794fc79f6664',
  'src/buildtools/linux64': {
    'packages': [
    {
      'package': 'gn/gn/linux-amd64',
      'version': 'git_revision:b2e3d8622c1ce1bd853c7a11f62a739946669cdd',
    }
    ],
    'dep_type': 'cipd',
    'condition': 'checkout_linux',
  },
  'src/buildtools/mac': {
    'packages': [
    {
      'package': 'gn/gn/mac-${{arch}}',
      'version': 'git_revision:b2e3d8622c1ce1bd853c7a11f62a739946669cdd',
    }
    ],
    'dep_type': 'cipd',
    'condition': 'checkout_mac',
  },
  'src/buildtools/win': {
    'packages': [
    {
      'package': 'gn/gn/windows-amd64',
      'version': 'git_revision:b2e3d8622c1ce1bd853c7a11f62a739946669cdd',
    }
    ],
    'dep_type': 'cipd',
    'condition': 'checkout_win',
  },

  'src/buildtools/clang_format/script':
    'https://chromium.googlesource.com/external/github.com/llvm/llvm-project/clang/tools/clang-format.git@99803d74e35962f63a775f29477882afd4d57d94',
  'src/buildtools/third_party/libc++/trunk':
    'https://chromium.googlesource.com/external/github.com/llvm/llvm-project/libcxx.git@8fa87946779682841e21e2da977eccfb6cb3bded',
  'src/buildtools/third_party/libc++abi/trunk':
    'https://chromium.googlesource.com/external/github.com/llvm/llvm-project/libcxxabi.git@cbf9455e837f39dac89f9e3365692e9251019d4e',
  'src/buildtools/third_party/libunwind/trunk':
    'https://chromium.googlesource.com/external/github.com/llvm/llvm-project/libunwind.git@cc80b4ac985d1c05a3d24dc96e4ffa7e98dbc535',


  'third_party':
  'https://github.com/DaHuMao/llvm-build.git@2eacd39252f667fc415096f980eeb7249edff795'
}

hooks = [
  {
    'name': 'clang_format_mac',
    'pattern': '.',
    'condition': 'host_os == "mac"',
    'action': [ 'download_from_google_storage',
    '--no_resume',
    '--platform=darwin',
    '--no_auth',
    '--bucket', 'chromium-clang-format',
    '-s', 'buildtools/mac/clang-format.sha1',
    ],
  },
]
