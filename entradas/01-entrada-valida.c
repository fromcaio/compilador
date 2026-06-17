int globalVar;
int values[10];

int sum(int a, int b) {
    int result = a + b;

    if (result > 10)
        result--;

    return result;
}

void main() {
    int i = 0;

    while (i < 10) {
        values[i] = sum(i, globalVar);
        i++;
    }

    for (; i > 0; i--) {
        values[i] = sum(values[i], i);
    }
}
