#define _MY_CONCAT3(a, b, c) a b c
#define _MY_CONCAT2(a, b) a b

#define _GET_OVERRIDE(_1, _2, _3, NAME, ...) NAME

#define MY_CONCAT(...) _GET_OVERRIDE(__VA_ARGS__, \
    _MY_CONCAT3, _MY_CONCAT2,)(__VA_ARGS__)

int main() {
    printf("3 args: %s\n", MY_CONCAT("a", "b", "c"));
    printf("2 args: %s", MY_CONCAT("a", "b"));
}