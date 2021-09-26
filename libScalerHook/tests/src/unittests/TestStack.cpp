#include <util/datastructure/Stack.h>


int main() {
    scaler::Stack<int> myStack;
    myStack.push(1);
    myStack.push(2);
    myStack.push(3);
    myStack.push(4);
    myStack.push(5);

    printf("%d\n",myStack.peek());
    myStack.pop();
    printf("%d\n",myStack.peek());
    myStack.pop();
    printf("%d\n",myStack.peek());
    myStack.pop();
    printf("%d\n",myStack.peek());
    myStack.pop();
    printf("%d\n",myStack.peek());
    myStack.pop();
    return 0;
}