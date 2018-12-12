/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *                  The shell acts as a task running in user mode. 
 *       The main function is to make system calls through the user's output.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this 
 * software and associated documentation files (the "Software"), to deal in the Software 
 * without restriction, including without limitation the rights to use, copy, modify, 
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit 
 * persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * 
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * */

#include "sched.h"
#include "test.h"
#include "stdio.h"
#include "screen.h"
#include "syscall.h"

static void disable_interrupt()
{
    uint32_t cp0_status = get_cp0_status();
    cp0_status &= 0xfffffffe;
    set_cp0_status(cp0_status);
}

static void enable_interrupt()
{
    uint32_t cp0_status = get_cp0_status();
    cp0_status |= 0x01;
    set_cp0_status(cp0_status);
}

static char read_uart_ch(void)
{
    char ch = 0;
    unsigned char *read_port = (unsigned char *)(0xbfe48000 + 0x00);
    unsigned char *stat_port = (unsigned char *)(0xbfe48000 + 0x05);

    while ((*stat_port & 0x01))
    {
        ch = *read_port;
    }
    return ch;
}

struct task_info task1 = {"task1", (uint32_t)&ready_to_exit_task, USER_PROCESS};
struct task_info task2 = {"task2", (uint32_t)&wait_lock_task, USER_PROCESS};
struct task_info task3 = {"task3", (uint32_t)&wait_exit_task, USER_PROCESS};

struct task_info task4 = {"task4", (uint32_t)&semaphore_add_task1, USER_PROCESS};
struct task_info task5 = {"task5", (uint32_t)&semaphore_add_task2, USER_PROCESS};
struct task_info task6 = {"task6", (uint32_t)&semaphore_add_task3, USER_PROCESS};

struct task_info task7 = {"task7", (uint32_t)&producer_task, USER_PROCESS};
struct task_info task8 = {"task8", (uint32_t)&consumer_task1, USER_PROCESS};
struct task_info task9 = {"task9", (uint32_t)&consumer_task2, USER_PROCESS};

struct task_info task10 = {"task10", (uint32_t)&barrier_task1, USER_PROCESS};
struct task_info task11 = {"task11", (uint32_t)&barrier_task2, USER_PROCESS};
struct task_info task12 = {"task12", (uint32_t)&barrier_task3, USER_PROCESS};

struct task_info task13 = {"SunQuan",(uint32_t)&SunQuan, USER_PROCESS};
struct task_info task14 = {"LiuBei", (uint32_t)&LiuBei, USER_PROCESS};
struct task_info task15 = {"CaoCao", (uint32_t)&CaoCao, USER_PROCESS};

static struct task_info *test_tasks[16] = {&task1, &task2, &task3,
                                           &task4, &task5, &task6,
                                           &task7, &task8, &task9,
                                           &task10, &task11, &task12,
                                           &task13, &task14, &task15};
static int num_test_tasks = 15;

void execute(uint32_t argc, char argv[][10])
{
    if(argc == 1)
    {
        if(!strcmp(argv[0], "ps"))
            sys_ps();
        else if(!strcmp(argv[0], "clear"))
            sys_clear();
        else
            printf("Unknown command!\n");
    }
    else if(argc == 2)
    {
        int pid = itoa((char *)argv[1]);
        if(!strcmp(argv[0], "exec"))
        {
            printf("exec process[%d]\n", pid-1);
            sys_spawn(test_tasks[pid-1]);
        }
        else if(!strcmp(argv[0], "kill"))
        {
            printf("kill process pid = %d\n", pid);
            sys_kill(pid);
        }
        else if(!strcmp(argv[0], "clear") && !strcmp(argv[1], "all"))
        {
            sys_clear_all();
        }
        else
            printf("Unknown command!\n");
    }
    else if(argc != 0)
        printf("Unknown command!\n");
}

void test_shell()
{
    char cmd[20];
    uint32_t i = 0, argc, j, k;
    char argv[3][10];

    /* terminal */
    sys_move_cursor(0, SCREEN_HEIGHT / 2);
    printf("-------------------- COMMAND --------------------\n");
    printf(">root@DJF_OS: ");
    
    while (1)
    {
        // read command from UART port
        disable_interrupt();
        char ch = read_uart_ch();
        enable_interrupt();

        // TODO solve command
        if(ch == 0 || (i == 0 && (ch == 0x7f || ch == 8)))
            continue;
        printf("%c", ch);
        if(ch != '\r')
        {
            if(ch == 8 || ch == 0x7f) // backspace
                i--;
            else 
                cmd[i++] = ch;
            continue;
        }
        else
        {
            cmd[i++] = ' ', cmd[i] = '\0';

            // argc: number of arguments, argv[]: arguments
            j = 0;
            if(cmd[j] == ' ')
                while(cmd[j] == ' ' && j < i)
                    j++;
            for(argc = k = 0; j < i; j++)
            {
                if(cmd[j] == ' ')
                {
                    argv[argc][k] = '\0';
                    argc++, k = 0;
                    while(cmd[j] == ' ' && j < i)
                        j++;
                }
                argv[argc][k++] = cmd[j];
            }

            execute(argc, argv);

            i = 0;
            printf(">root@DJF_OS: ");
        }
    }
}