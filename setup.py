from skbuild import setup

setup(
  name='Packaide',
  version='2.0',
  description='Fast and robust 2D nesting',
  author='Daniel Anderson & Adrian Sy',
  author_email='dlanders@cs.cmu.edu',
  url='https://github.com/DanielLiamAnderson/Packaide',
  license='GPL',
  packages=['packaide'],
  package_dir={'': 'python'},
  cmake_args=['-DCMAKE_BUILD_TYPE=Release'],
  cmake_install_dir='python/packaide',
  python_requires=">=3.6"
)
