// Operadores relacionais e lógicos: < > <= >= == != && || !
int compara() {
    int a;
    int b;
    bool r;
    a = 5;
    b = 8;
    r = a < b;
    r = a > b;
    r = a <= b;
    r = a >= b;
    r = a == b;
    r = a != b;
    r = (a < b) && (b > 0);
    r = (a < b) || (b < 0);
    r = !r;
    return a;
}
