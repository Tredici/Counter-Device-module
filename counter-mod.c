/* Questo semplicissimo modulo kernel serve a fornire
 * un device virtuale per ottenere dei numeri progressivi
 * ad ogni lettura unici per tutti i processi attivi su
 * una data macchina.
 * Implementa di fatto un contatore.
 * Il contatore è resettato dopo ogni riavvio.
 *
 * Per ottenere lo scopo non si fa uso di complicazioni
 * quali mutex e spin lock ma si ricorre a più semplici
 * variabili atomiche.
 * Usa:
 *  atomic_long_t
 *  atomic_long_inc_return
 *
 * Pergarantire l'unicità della lettura uso il campo offset:
 *  +potrà valere solo 0 o 1:
 *      -0: si può effettuare una lettura
 *      -1: dato già letto
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

#include <linux/atomic.h>
#include <linux/miscdevice.h>   /* per struct miscdevice */
#include <linux/fs.h>           /* per struct file_operations */
#include <linux/string.h>

/* licenza */
MODULE_LICENSE("GPL");

/* sempre utile */
#define MIN(x,y) ((x)<(y) ? (x) : (y))

/* contatore - inizializzato a 0 */
static atomic_long_t global_counter;


/* gestore della lettura */
static ssize_t dev_read(struct file *file, char __user *buffer, size_t bufLen, loff_t *offset)
{
    unsigned long long int counter;
    char test[40];
    int err;
    size_t to_transfer;

    if (*offset != 0) /* già letto */
        return 0;

    /* lettura atomica */
    counter = atomic_long_inc_return(&global_counter) - 1;

    /* mette nella stringa il numero */
    snprintf(test, 40, "%llu\n", counter);

    /* quanti byte trasferire */
    to_transfer = MIN(strlen(test), bufLen);

    err = copy_to_user(buffer, test, to_transfer);
    if (err)
        return -EINVAL;

    *offset = 1;

    return (ssize_t)to_transfer;
}

/* https://www.kernel.org/doc/html/latest/filesystems/vfs.html?highlight=struct%20file_operations */
static struct file_operations fops = {
    .owner  =   THIS_MODULE,
    .read   = &dev_read
};

static struct miscdevice misc = {
    MISC_DYNAMIC_MINOR, "test-counter", &fops
};

static int __init counter_init(void)
{
    printk("Loading counter module...\n");
    return misc_register(&misc);
}
static void __exit counter_exit(void)
{
    printk("Unloading counter module...\n");
    misc_deregister(&misc);
}

/* avvio e chiusura */
module_init(counter_init);
module_exit(counter_exit);
