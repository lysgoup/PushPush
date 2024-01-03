#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <time.h> 
#include <stdbool.h>
#include <pthread.h> 
#include <sys/socket.h> 
#include <cjson/cJSON.h> 


#define CELL_SIZE 16

char nickname[4][9];
char ip[16];
int port;


GtkWidget *score_label;
int score = 0;

void update_score() {
    gchar *score_str = g_strdup_printf("Score: %d", score);
    gtk_label_set_text(GTK_LABEL(score_label), score_str);
    g_free(score_str);
}

void button_clicked(GtkWidget *widget, gpointer data) {
    const char *direction = (const char *)data;

    score++;
    update_score();
    
    g_print("Button clicked: %s\n", direction);
}

void on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
    switch (event->keyval) {
        case GDK_KEY_Up:
            g_print("Up key pressed\n");
            break;
        case GDK_KEY_Down:
            g_print("Down key pressed\n");
            break;
        case GDK_KEY_Left:
            g_print("Left key pressed\n");
            break;
        case GDK_KEY_Right:
            g_print("Right key pressed\n");
            break;
        default:
            break;
    }
}

//make new sock and connection with ip,port and return the sock_fd
// if it was succeed return 0, otherwise return 1
int makeConnection(){

   int conn;
   struct sockaddr_in serv_addr; 
   conn = socket(AF_INET, SOCK_STREAM, 0) ;
   
   if (conn <= 0) {
      fprintf(stderr,  " socket failed\n");
      return 1;
   } 

   memset(&serv_addr, '\0', sizeof(serv_addr)); 
   serv_addr.sin_family = AF_INET; 
   serv_addr.sin_port = htons(port); 
   if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0) {
      fprintf(stderr, " inet_pton failed\n");
      return 1;
   }
   if (connect(conn, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
      fprintf(stderr, " connect failed\n");
      return 1;
   }

   return conn;
}

// if it was succeed return 0, otherwise return 1
int send_bytes(int fd, void * buf, size_t len) {
    char * p = buf ;
    size_t acc = 0 ;

    while (acc < len) {
        size_t sent ;
        sent = send(fd, p, len - acc, MSG_NOSIGNAL) ;
        if (sent == 0)
            return 1 ;
        p += sent ;
        acc += sent ;
    }
    return 0 ;
}

// if it was succeed return 0, otherwise return 1
int recv_bytes(int fd, void * buf, size_t len){
    char * p = buf ;
    size_t acc = 0 ;

    while (acc < len) {
        size_t recved ;
        recved = recv(fd, p, len - acc, MSG_NOSIGNAL) ;
        if (sent == 0)
            return 1 ;
        p += recved ;
        acc += recved ;
    }
    return 0 ;
}

// if it was succeed return 0, otherwise return 1
int read_map(int fd, void * buf, size_t len) {
    

    return 0;
}

void * input (void * arg) {


}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: ./%s <IP> <port> <nickname>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    else if (strlen(argv[1]) > 8) {
        fprintf(stderr, "Nicknames must be less than 9 characters\n");
        exit(EXIT_FAILURE);
    }

    strcpy(ip, argv[1]);
    strcpy(port, argv[2]);

    char name[9];
    strcpy(name, argv[3]);

    // make connection
    int send_conn;
    if ((send_conn = makeConnection()) == 1) {
        fprintf(stderr, "connect failed\n");
        exit(EXIT_FAILURE);
    }

    // send nickname to server
    if (send_bytes(send_conn, name, 9) == 1) {
        fprintf(stderr, "send nickname failed\n");
        exit(EXIT_FAILURE);
    }

    close(send_conn);

    int recv_conn;
    if ((recv_conn = makeConnection()) == 1) {
        fprintf(stderr, "connect failed\n");
        exit(EXIT_FAILURE);
    }

    // receive username list
    int i;
    for (i = 0; i < 4; i++) {
        if (read_bytes(recv_conn, nickname[i], 9) == 1) {
            fprintf(stderr, "send nickname failed\n");
        exit(EXIT_FAILURE);
        }
    }

    // read json map
    // have to fix
    char * map;
    if (read_map(recv_conn, map, 10) != 0) {
        fprintf(stderr, "read map failed\n");
        exit(EXIT_FAILURE);
    }

    // receive time stamp

    close(recv_conn);

    // make threads
    pthread_t input_pid;
    if (pthread_create(&input_pid, NULL, (void*)input, NULL)) {
        perror("make input thread failed\n");
        exit(EXIT_FAILURE);
    }








    GtkWidget *window;
    GtkWidget *table;
    GtkWidget *event_box;
    GtkWidget *hbox;

    GtkWidget *frame;

    GtkWidget *base;
    GtkWidget *user;

    GtkWidget *cell[NUM_ROWS][NUM_COLS];

    gtk_init(&argc, &argv);

    GdkColor color;
    gdk_color_parse("#F8EDFF", &color);
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_widget_modify_bg(window, GTK_STATE_NORMAL, &color);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_window_set_title(GTK_WINDOW(window), "PushPush");

    // Create a vbox to hold all the widgets
    hbox = gtk_hbox_new(FALSE, 5);
    gtk_container_add(GTK_CONTAINER(window), hbox);

    // Create a table for the cells
    table = gtk_table_new(NUM_ROWS, NUM_COLS, TRUE);
    gtk_container_set_border_width(GTK_CONTAINER(table), 0);  // Set padding to 0
    gtk_box_pack_start(GTK_BOX(hbox), table, TRUE, TRUE, 0);


    // Create a frame for additional content
    gdk_color_parse("#FFF6F6", &color);
    frame = gtk_frame_new("Score");
    gtk_widget_modify_bg(frame, GTK_STATE_NORMAL, &color);
    gtk_box_pack_end(GTK_BOX(hbox), frame, FALSE, FALSE, 0);

    // Create a label for the score and add it to the frame
    score_label = gtk_label_new("USER 1: ");
    gtk_container_add(GTK_CONTAINER(frame), score_label);

    srand(time(NULL));
    int random;
    bool base_flag = false;
    bool user_flag = false;


    for (int i = 0; i < NUM_ROWS; i++) {
        for (int j = 0; j < NUM_COLS; j++) {
            event_box = gtk_event_box_new();

            random = rand() % 100;

            if (i == 0 || j == 0 || i == NUM_ROWS - 1 || j == NUM_COLS - 1 || random % 21 == 0) {
                // Create a GtkImage and set it to your image file
                cell[i][j] = gtk_image_new_from_file("bank16.png");
                
            } else if ((base_flag == false) && (random % 33 == 0)) {
                base_flag = true;
                cell[i][j] = gtk_image_new_from_file("bin116.png");
            } else if (random % 50 == 0) {
                cell[i][j] = gtk_image_new_from_file("banana16.png");
            } else if ((base_flag == true) && user_flag == false) {
                if (j != 59) {
                    cell[i][j] = gtk_image_new_from_file("mario16.png");
                    user_flag = true;
                }
            } else {
                cell[i][j] = gtk_image_new_from_file("marble16.png");
            }
            gtk_container_add(GTK_CONTAINER(event_box), cell[i][j]);
            gtk_widget_set_size_request(event_box, CELL_SIZE, CELL_SIZE);
            gtk_table_attach_defaults(GTK_TABLE(table), event_box, j, j + 1, i, i + 1);
        }
    }

    // Update the score label
    update_score();

    g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // Connect the key press event to the on_key_press function
    g_signal_connect(G_OBJECT(window), "key-press-event", G_CALLBACK(on_key_press), NULL);

    gtk_widget_show_all(window);

    gtk_main();

    return 0;
}
