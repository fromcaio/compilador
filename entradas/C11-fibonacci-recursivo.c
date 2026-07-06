// Exemplo interessante: Fibonacci recursivo
// Combina: função, recursão (chamada a si mesma), condicional, retorno
int fib(int n) {
    if (n <= 1) {
        return n;
    }
    return fib(n - 1) + fib(n - 2);
}

void main() {
    int resultado;
    resultado = fib(10);
}
