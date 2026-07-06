// Exemplo interessante: Fibonacci iterativo preenchendo um vetor
// Combina: vetor, laço for, condicional e operações aritméticas
int fib[10];

void main() {
    int i;
    fib[0] = 0;
    fib[1] = 1;
    for (i = 2; i < 10; i++) {
        fib[i] = fib[i - 1] + fib[i - 2];
    }
}
