#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>

int main() 
{
    /* 输入的命令行 */
    char cmd[256];
    /* 命令行拆解成的各部分，以空指针结尾 */
    //这是一个字符指针数组!
    char *args[128];
    while (1) 
    {
        /* 提示符 */
        printf("$ ");
        fflush(stdin);
        fgets(cmd, 256, stdin);
        /* 清理结尾的换行符 */
        int i;
        for (i = 0; cmd[i] != '\n'; i++);
        //
        cmd[i] = '\0';
        /* 拆解命令行 */
        //必要的步骤！
        args[0] = cmd;
        for (i = 0; *args[i]; i++)
            for (args[i+1] = args[i] + 1; *args[i+1]; args[i+1]++)
            //此处将for 改为while    
            //待测试
                while (*args[i+1] == ' ') {
                    *args[i+1] = '\0';
                    args[i+1]++;
                //此处有break语句！！
                    break;
                }
        // 之后均无命令
        args[i] = NULL;

        /* 没有输入命令 */
        if (!args[0])
            continue;

        /* 内建命令 */
        if (strcmp(args[0], "cd") == 0) 
        {
            if (args[1])
                chdir(args[1]);
            continue;
        }
        if (strcmp(args[0], "pwd") == 0) 
        {
            char wd[5000];//wd[4090]
            puts(getcwd(wd, 5000));
            continue;
        }
        if (strcmp(args[0], "exit") == 0)
            return 0;

        /* 外部命令 */
     //管道命令 若有 “|” 则执行
     //用于记录管道之间的代码
        char *args_1[128];
        for(i = 0;args[i];i++)
        {
            args_1[i] = args[i];
        }
        /* 清理 |  
        for (i = 0; args_1[i] != NULL; i++);
            if(strcmp(args_1[i], "|") == 0)
                args_1[i] = NULL ;*/
        /* 拆解命令行 */
        args_1[0] = args[0];
        for (i = 0; *args_1[i]; i++)
            for (args_1[i+1] = args_1[i] + 1; args_1[i+1]; args_1[i+1]++)
            //此处将for 改为while    
            //待测试
                if (*args_1[i+1] == '|') {
                    args_1[i+1] = NULL ;
                    args_1[i+1]++;
                //此处有break语句！！
                    break;
                }
        args_1[i] = NULL;
        if(args_1[1])
        //进入管道命令
        {
            int pipefd[2] ;
        //    char buff[1024];
            //创建一个字程序，子程id号为pid的值
            pid_t pid = fork();
            pipe(pipefd);
            for (int i = 1; args_1[i] ; ++i)
            {
                //创建管道失败
                if(pipe(pipefd) == -1)
                 {
                    perror("pipe error:");
                    exit(EXIT_FAILURE);
                }

                //创建子程序失败
                if(pid == -1 )
                {
                    perror("fork  error:");
                    exit(EXIT_FAILURE);
                }
                else if (pid == 0) 
                //成功创建子程序
                {
                    /* 子进程 */
                    close(stdout);
                    dup2(pipefd[1],1);
                    close(pipefd[0]);
                    execvp(args_1[0], args_1);
                    /* execvp 执行失败则直接返回-1, 失败原因存于errno 中*/
                    if(execvp(args_1[0], args_1) == -1)
                        return 255;
                }
                else
                /* 父进程 */
                {
                    close(stdin);
                    dup2(pipefd[0],0);
                    close(pipefd[1]);
                    execvp(args_1[0], args_1);
                    /* execvp 执行失败则直接返回-1, 失败原因存于errno 中*/
                    if(execvp(args_1[0], args_1) == -1)
                        return 255;
                }
                

                wait(NULL);/* code */
            }
            
        }
        

    }
}



