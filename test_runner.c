#include "test_functions.h"
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

#define RED "\x1b[31m"
#define GREEN "\x1b[32m"
#define YELLOW "\x1b[33m"
#define BLUE "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN "\x1b[36m"
#define YELLOW "\x1b[33m"
#define RESET "\x1b[0m"

#define N_ELEMENTS(X) (sizeof(X) / sizeof(*(X)))

typedef enum _status {
  PASS = 1,
  FALHA = 0,
  ERRO = -1,
  NONE = -2
} status;

char* status_to_str(status s, int colored) {
  if (s == -1) return colored ? YELLOW "[ERRO]" RESET : "[ERRO]";
  else if (s == 0) return colored ? RED "[FALHA]" RESET : "[FALHA]";
  else if (s == 1) return colored ? GREEN "[PASS]" RESET : "[PASS]";
  else return "NONE";
}

typedef struct {
  int id;
  char fn_name[128];
  void (*fn)(void);
  int timeout;
  double duration;
  status status;
  char output[4096];
  char stderr_output[4096];
  char end_reason[128];
} test;

test test_init(func fn, int id) {
  test t = {id, "unknown", fn, 6, 0, NONE, {0}, {0}, "NONE"};
  return t;
}

static test *tests; // shared IPC tests array

void print_test(test t) {
  fprintf(stderr, "\n%s TEST %d - %s - duration %fs - %s\n", status_to_str(t.status, 1), t.id, t.fn_name, t.duration, t.end_reason);
  if (t.status == FALHA) {
    printf(RED "Captured error: %s" RESET, t.stderr_output);
    printf("\n");
  }
  printf("OUTPUT:\n");
  printf("%s", t.output);
}

void write_html_report(int n) {
  FILE *f = fopen("report.html", "w");
  char *html_header = "<!DOCTYPE html><html><head></head><body><h1> TEST REPORT - Fred's Lab </h1>";
  char *html_footer = "</body></html>";
  
  lab_printf(CYAN "Writing HTML report" RESET);
  if (f == NULL) {
    printf("Error opening file!\n");
    exit(1);
  }
  
  fprintf(f,"%s",html_header);
  for (int i = 0; i < n; i++) {
    fprintf(f, "<li>%s TEST %d - %s - duration %fs - %s </li><br>",
      status_to_str(tests[i].status, 0), tests[i].id, tests[i].fn_name, tests[i].duration, tests[i].end_reason
    );
    // fprintf(f, "<p>%s</p>", tests[i].output); // supressed output on html report
  }
  fprintf(f, "%s", html_footer);
  lab_printf(CYAN "HTML report written!" RESET);
}

void sigint_handler(int code) {
  char r[1];
  lab_printf(YELLOW "Are you sure you want to cancel the program?" RESET);
  printf("[y] Yes\n[n] No\n");
  scanf("%c", r);
  if (r[0] == 'y') {
    printf("Bye then 😭\n");
    exit(0);
  } else {
    printf("Lets keep going!\n");
    return;
  }
}

void handle_test(test *t, int id) {
  int status, end_signal, p[2], p2[2], error_in_fd;
  char buf, buf_err, child_output[2048];
  int c = 0, c2 = 0;
  struct timeval tv1, tv2;

  // cria pipes para process intercom
  pipe(p);
  pipe(p2);
  gettimeofday(&tv1, NULL);

  pid_t child_pid = fork();

  // PARENT
  if (child_pid != 0) {
    dup2(p[0], 0); // directiona saida do pipe pra entrada do handler
    error_in_fd = dup2(p2[0], id + 10); // directiona saida do pipe pra entrada do handler
    
    close(p[1]);
    close(p2[1]);

    waitpid(child_pid, &status, 0);
    gettimeofday(&tv2, NULL);
    t->duration = (double)(tv2.tv_usec - tv1.tv_usec) / 1000000 + (double)(tv2.tv_sec - tv1.tv_sec);

    // reads output from child task
    while (read(STDIN_FILENO, &buf, sizeof(buf)) > 0) {
      child_output[c] = buf;
      c++;
    }
    child_output[c] = '\0';
    
    // writes output to memory buffer
    memset(t->output, 0, strlen(t->output));
    sscanf(child_output, "%s\n", t->fn_name);
    strcpy(t->output, child_output);

    // Teste terminou normalmente
    if (WIFEXITED(status)) {
      lab_printf("[TEST %d] Teste terminou normalmente com código de saída %d!", id, WEXITSTATUS(status));
      // lab_printf("Child output: %s", child_output); // printa output capturado pelo pipe;
      if (WEXITSTATUS(status) == 1) {
        lab_printf(RED "Lab assert FAILED ❌" RESET);
        t->status = FALHA;
        strcpy(t->end_reason, "LAB_ASSERT RETURNED FALSE");

        while (read(error_in_fd, &buf_err, sizeof(buf_err)) > 0) {
          t->stderr_output[c2] = buf_err;
          c2++;
        }
        t->stderr_output[c2] = '\0';

        printf("TASK %d CAPTURED ERROR : %s\n",id, t->stderr_output);

      } else if (WEXITSTATUS(status) == 0) {
        lab_printf(GREEN "Lab assert PASSED ✅" RESET);
        t->status = PASS;
        strcpy(t->end_reason,"LAB_ASSERT RETURNED TRUE");
      }

    // Houve um sinal
    } else if (WIFSIGNALED(status)){ 
      end_signal = WTERMSIG(status);
      lab_printf("[TEST %d] Teste %d encerrou pelo sinal %d",id, id, end_signal);

      // Lidar com sinais específicos
      if (end_signal == SIGSEGV || end_signal == SIGBUS) {
        lab_printf(YELLOW "Segfault! ⚠️" RESET);
        strcpy(t->end_reason, "SEGMENTATION FAULT");
      } else if (end_signal == SIGALRM) {
        lab_printf(YELLOW "Timeout! ⏰" RESET);
        strcpy(t->end_reason, "TIMEOUT");
      } else if (end_signal == SIGTERM) {
        lab_printf(YELLOW "Process terminated! 🔪 " RESET);
        strcpy(t->end_reason, "PROCESS TERMINATED");
      }
      t->status = ERRO;
    }
  
  // CHILD
  } else {
    dup2(p[1], 1); // directiona saida padrao do filho pro pipe
    dup2(p2[1], 2); // directiona erro padrao do filho pro pipe
    close(p[0]);
    close(p2[0]);

    alarm(t->timeout); // timeout for child process
    t->fn();  // test to be run;
  }
  return;
}

// - MAIN FUNCTION

int main(int argc, char **argv) {
  int test_count = N_ELEMENTS(test_list);
  int pass_count = 0, error_count = 0, fail_count = 0;
  struct sigaction sint;
  int html_report_requested = 0;
  struct timeval tv1, tv2;

  // test tests[test_count];
  tests = mmap(NULL, sizeof(*tests)*test_count, PROT_READ | PROT_WRITE,
                  MAP_SHARED | MAP_ANONYMOUS, -1, 0);

  printf(YELLOW "\n------ 😂 Welcome to Fred's TestLab 😂 ------\n\n" RESET);
  lab_printf("There are %d tests to run!", test_count);

  // Checks for -html report tag
  if (argc == 2) {
    if (strcmp(argv[1], "-html") == 0){
      lab_printf("Requested HTML report!");
      html_report_requested = 1;
    } else {
      lab_printf("invalid flag m8");
    }
  }

  gettimeofday(&tv1, NULL);
  // For each we'll spawn a new test instance and pass it to a handler
  for (int i = 0; i < test_count; i++) {
    tests[i] = test_init(test_list[i], i + 1);
    // test handlers are also child processes
    if (fork() == 0) {
      handle_test(&tests[i], tests[i].id);
      exit(0);
    }
  }

  lab_printf("Done spawning test handlers");
  
  // listens to CTRL+C
  sint.sa_handler = sigint_handler;
  sigemptyset(&sint.sa_mask);
  sigaddset(&sint.sa_mask, SIGTERM);
  sigaction(SIGINT, &sint, NULL);

  printf("\n" BLUE "------------------- Log ---------------------" RESET "\n\n");
  
  for (int j = test_count - 1; j != -1; j--) {
    waitpid(-1, NULL, 0);
    lab_printf("%d Tests left!", j);
  }

  lab_printf("Tests are finished! 🤘");
  gettimeofday(&tv2, NULL);

  printf(CYAN "\n----------------- Summary -------------------" RESET "\n");

  for (int k = 0; k < test_count; k++) {
    print_test(tests[k]);
    if (tests[k].status == -1) {error_count++;}
    if (tests[k].status == 0) {fail_count++;}
    if (tests[k].status == 1) {pass_count++;}
  }

  printf("\n✅  " GREEN "PASS: " RESET "%d/%d\n", pass_count, test_count);
  printf("❌  " RED "FAIL: " RESET "%d/%d\n", fail_count, test_count);
  printf("⚠️   " YELLOW "ERRO: " RESET "%d/%d\n", error_count, test_count);
  printf("---------------\n🏁  TESTS %d/%d\n", test_count, test_count);
  double duration = (double)(tv2.tv_usec - tv1.tv_usec) / 1000000 + (double)(tv2.tv_sec - tv1.tv_sec);
  printf("⏱️   DURATION: %fs\n", duration);

  if (html_report_requested) {
    write_html_report(test_count);
  }

  printf("\nThank you for using Fred's TestLab! 👍👍👍🔥🔥\n");
  exit(0);
}
