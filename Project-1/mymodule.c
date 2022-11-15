#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/pid.h>
#include <linux/sched.h>
#include <linux/slab.h>

// Meta Information
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Çisem Özden & Gülbarin Maçin");
MODULE_DESCRIPTION("A module that knows how to greet");


/*
 * module_param(foo, int, 0000)
 * The first param is the parameters name
 * The second param is it's data type
 * The final argument is the permissions bits,
 * for exposing parameters in sysfs (if non-zero) at a later stage.
 */

int my_pid=0;
module_param(my_pid, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(my_pid, "PID: \n");
void traverse(struct task_struct *task, int **process_pid, int **creation_time);
/* This function is called when the module is loaded. */
int simple_init(void)
{
    printk(KERN_INFO "Loading PSVIS Module\n");
    // checking the given PID
    if (my_pid < 0)
    {
        return -1 ;
    }
    else // valid PID
    {
        struct task_struct *task;
       	int **process_pid = kmalloc(150 * sizeof(int*), GFP_KERNEL);
        int **creation_time = kmalloc(150 * sizeof(int*), GFP_KERNEL);
        int i=0;
        while(i<=150) {
            process_pid[i] = kmalloc(150 * sizeof(int), GFP_KERNEL);
            creation_time[i] = kmalloc(150 * sizeof(int), GFP_KERNEL);
            i++;
        }
        // finding the task with given PID
        task = pid_task(find_get_pid(my_pid), PIDTYPE_PID);
        if (task != NULL) {
            process_pid[0] = (int) task->pid;
            creation_time[0] = (int) task->start_time;
            traverse(task,process_pid,creation_time);
        }
       
    }

    return 0;
}

/* This function is called when the module is removed. */
void simple_exit(void)
{
    printk(KERN_INFO "PSVIS Module is removing\n");
}


void traverse(struct task_struct *task, int **process_pid, int **creation_time)
{  
    struct task_struct *child;
    struct list_head *list;
   
    int j=0;
    list_for_each(list, &task->children) {
        child = list_entry(list, struct task_struct, sibling);
        process_pid[j] = (int) child->pid;
        creation_time[j] = (int) child->start_time;
        traverse(child, process_pid, creation_time);
        j++;
    }
    printk(KERN_CONT "PID: %d, Creation Time: %d\n", task->pid, task->start_time);

}


module_init(simple_init);
module_exit(simple_exit);
