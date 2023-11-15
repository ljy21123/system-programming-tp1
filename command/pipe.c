#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>    // open

int main(int argc, char *argv[]) {
    int start = 0;
    if (argc < 2){
        printf("명령어를 확인해 주세요!");
        return 0;
    }  

    bool has_pipe = false;
    int output_redirection = -1;
    int input_redirection = -1;
    int pipe_index = -1;

    // 리다이렉션 심볼의 위치를 탐색
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "<") == 0) {
            input_redirection = i;
        } else if (strcmp(argv[i], ">") == 0) {
            output_redirection = i;
        } else if (strcmp(argv[i], "|") == 0) {
            has_pipe = true;
            pipe_index = i;
            break; // 첫 번째 파이프를 만나면 반복문 탈출
        }
    }
    

    if(strcmp(argv[0], "./") == 0) // 만약 실행파일 이름이라면
        start++;   

    // 파이프가 있다면
    if (has_pipe){
        // 첫 번째 배열 동적 할당 및 값 복사
        char **firstArray = (char **)malloc((pipe_index + 1) * sizeof(char *));
        for (int i = 0, j = start; j < pipe_index; i++, j++) {
            firstArray[i] = argv[j];
        }
        firstArray[pipe_index] = NULL; // 배열 끝을 나타내기 위해 NULL 추가

        // 두 번째 배열 동적 할당 및 값 복사
        char **secondArray = (char **)malloc((argc - pipe_index) * sizeof(char *));
        for (int i = pipe_index + 1, j = 0; i < argc; i++, j++) {
            secondArray[j] = argv[i];
        }
        secondArray[argc - pipe_index - 1] = NULL;
        
        // printf("첫번째 명령어\n");
        // for(int i=0; i<pipe_index; i++)
        //     printf("%s\n", firstArray[i]);
        
        // printf("두번째 명령어\n");
        // for(int i=0; i<argc - pipe_index;i++)
        //     printf("%s\n", secondArray[i]);

        // 파이프 생성
        int pipe_fd[2];
        if (pipe(pipe_fd) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }

        // 첫 번째 자식 프로세스 생성
        pid_t pid1 = fork();
        if (pid1 == 0) {
            // 표준 출력을 파이프의 쓰기 단으로 리디렉션
            dup2(pipe_fd[1], STDOUT_FILENO);
            close(pipe_fd[0]);
            close(pipe_fd[1]);

             // 명령어 실행파일 경로를 저장할 변수
            char command_path[256];
            // 입력된 명령어에 실행파일 경로 추가
            snprintf(command_path, sizeof(command_path), "./command/%s", firstArray[0]);
            // 첫 번째 명령어 실행
            execvp(command_path, firstArray);
            // execvp(firstArray[0], firstArray);
            perror("execvp first command");
            exit(EXIT_FAILURE);
        } else if (pid1 < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        close(pipe_fd[1]); // 부모는 파이프의 쓰기 단을 사용하지 않음

        waitpid(pid1, NULL, 0);
        // 두 번째 자식 프로세스 생성
        pid_t pid2 = fork();
        if (pid2 == 0) {
            // 표준 입력을 파이프의 읽기 단으로 리디렉션
            dup2(pipe_fd[0], STDIN_FILENO);
            close(pipe_fd[0]);
            close(pipe_fd[1]);

            // 명령어 실행파일 경로를 저장할 변수
            char command_path[256];
            // 입력된 명령어에 실행파일 경로 추가
            snprintf(command_path, sizeof(command_path), "./command/%s", secondArray[0]);
            // 두 번째 명령어 실행
            execvp(command_path, secondArray);

            // execvp(secondArray[0], secondArray);

            perror("execvp second command");
            exit(EXIT_FAILURE);
        } else if (pid2 < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        // 부모 프로세스
        close(pipe_fd[0]); // 부모는 파이프의 읽기 단을 사용하지 않음

        // 두 자식 프로세스의 종료를 기다림

        waitpid(pid2, NULL, 0);

        free (firstArray);
        free (secondArray);
        return 0;
    }

    // for (int i=0; i<argc; i++)
    //     fprintf(stderr,"%s\n", argv[i]);
    // 입력 리다이렉션 처리
    if (input_redirection != -1) {
        // 입력 리다이렉션 처리
        // 파일을 읽기 전용으로 열고 파일 디스크립터 저장
        int fd = open(argv[input_redirection + 1], O_RDONLY);
        if (fd < 0) {
            perror("open");
            return 0;
        }
        // fd를 표준입력으로 설정
        dup2(fd, 0);
        close(fd);
        // 토큰에서 리다이렉션 토큰과 파일을 제거
        argv[input_redirection] = NULL;
        argv[input_redirection + 1] = NULL;
    }

    // 출력 리다이렉션 처리
    if (output_redirection != -1) {
        // 출력 리다이렉션 처리
        // 파일을 여는데 쓰기 전용이며, 없으면 생성하고, 이미 파일이 있을때 존재하는 내용을 지운다
        int fd = open(argv[output_redirection + 1], O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd < 0) {
            perror("open");
            return 0;
        }
        dup2(fd, 1);
        close(fd);
        // 토큰에서 리다이렉션 토큰과 파일을 제거
        argv[output_redirection] = NULL;
        argv[output_redirection + 1] = NULL;
    }

    // for (int i=0; i<argc; i++)
    //     fprintf(stderr,"%s\n", argv[i]);
    // 자식 프로세스 생성
    int pid = fork();

    if (pid == 0){ // 자식 프로그램
        // 명령어 실행파일 경로를 저장할 변수
        char command_path[256];
        // 입력된 명령어에 실행파일 경로 추가
        snprintf(command_path, sizeof(command_path), "./command/%s", argv[0]);
        execvp(command_path, argv); // 실행 프로그램을 실행함
        // 실행파일이 없다면 오류출력
        fprintf(stderr, "%s: Command not found\n", command_path);
    } else if (pid > 0){ // 부모 프로그램
        wait((int *) 0); // 자식 프로그램 대기
    } else{
        perror("fork failed");
    }

    return 0;
}
