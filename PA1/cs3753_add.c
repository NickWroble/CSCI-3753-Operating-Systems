#include<linux/init.h>
#include<linux/module.h>
#include <linux/uaccess.h>

MODULE_AUTHOR("Nicholas Wroblewski");
MODULE_LICENSE("GPL");

asmlinkage long sys_cs3753_add(int number1, int number2, int *result){
    int sum;
    printk("Adding %d and %d.\n", number1, number2);
    sum = number1 + number2;
    printk("Sum is %d\n", sum);
    copy_to_user(result, &sum, sizeof(int));
    return 0;
}