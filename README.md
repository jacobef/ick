## ick - the Imacculate C Kompiler
Currently just does trigraphs and tokenization.
Hopefully the preprocessor will be complete in a few months.

### Building (only on Linux/macOS/WSL)

```shell
git clone https://github.com/jacobef/ick
cd ick
mkdir build
cd build
cmake ..
make
````

### Usage

`./ick filename.c`

You can test it with:
`./ick ../test/compile_this.c`
