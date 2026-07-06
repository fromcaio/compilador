// Laço while: soma de 1 até 10
int soma_while() {
    int i;
    int total;
    i = 1;
    total = 0;
    while (i <= 10) {
        total = total + i;
        i++;
    }
    return total;
}
