// break e continue dentro de um laço
int filtra() {
    int i;
    int total;
    total = 0;
    for (i = 0; i < 20; i++) {
        if (i % 2 == 0) {
            continue;
        }
        if (i > 10) {
            break;
        }
        total = total + i;
    }
    return total;
}
