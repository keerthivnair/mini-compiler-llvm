func multiply(a, b) {
    return a * b;
}

func calculate_secret(x) {
    y = 5;
    if (x > 10) {
        return multiply(x, y);
    }
    return x + y;
}

func main() {
    // Try changing these values!
    my_value = 12;
    result = calculate_secret(my_value);
    
    print(result);
    return 0;
}
