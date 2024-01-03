#include <gtk/gtk.h>

#define ROWS 20
#define COLS 20
#define BARRIER 1
#define PLAYER 2
#define DIA 3
#define GOAL 4

typedef struct Location{
    int row;
    int col;
}Location;

int board[ROWS][COLS];
int count;
GtkWidget *window;
GtkWidget *main_grid;
GtkWidget *play_button;
GtkWidget *board_grid = NULL;
GtkWidget *label;
GtkWidget *image;
GtkWidget *button_box;
GtkWidget *btn_up, *btn_down, *btn_left, *btn_right, *btn_restart;
Location player;
char msg_info[16];

void init_board();
void update_board_grid();
void play_button_clicked(GtkWidget *play_button, gpointer user_data);
void on_key_press(GtkWidget *window, GdkEventKey *event, gpointer user_data);
void up_button_clicked(GtkWidget *up_button, gpointer user_data);
void down_button_clicked(GtkWidget *down_button, gpointer user_data);
void left_button_clicked(GtkWidget *left_button, gpointer user_data);
void right_button_clicked(GtkWidget *right_button, gpointer user_data);
void restart_button_clicked(GtkWidget *restart_button, gpointer user_data);

void init_board(){
    memset(board, 0, sizeof(board));
    for(int i=0;i<ROWS;i++){
        for(int j=0;j<COLS;j++){
            if(i==0 || j==0 || i==ROWS-1 || j==COLS-1){
                board[i][j] = BARRIER;
            }
            else{
                board[i][j] = 0;
            }
        }
    }
    board[3][7] = DIA;
    board[3][3] = DIA;
    board[5][2] = DIA;
    board[7][7] = DIA;
    board[3][2] = BARRIER;
    board[4][2] = BARRIER;
    board[6][5] = BARRIER;
    board[8][1] = GOAL;
    board[1][1] = PLAYER;
    player.row = 1;
    player.col = 1;
    count = 4;
}


void update_board_grid(){
    if(board_grid != NULL){
        gtk_widget_destroy(board_grid);
    }
    board_grid = gtk_grid_new();
    gtk_widget_set_size_request(board_grid, 200, 200);
    gtk_grid_attach(GTK_GRID(main_grid), board_grid, 0, 0, 1, 1);
    
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            if(board[i][j]==BARRIER){
                image = gtk_image_new_from_file("./image/ground16.png");
            }
            else if(board[i][j]==DIA){
                image = gtk_image_new_from_file("./image/dia16.png");
            }
            else if(board[i][j]==GOAL){
                image = gtk_image_new_from_file("./image/goal16.png");
            }
            else if(board[i][j]==PLAYER){
                image = gtk_image_new_from_file("./image/player16.png");
            }
            else{
                image = gtk_image_new_from_file("./image/background16.png");
            }
            // gtk_widget_set_size_request(image,20,20);
            gtk_grid_attach(GTK_GRID(board_grid), image, j, i, 1, 1);
        }
    }
}

void play_button_clicked(GtkWidget *play_button, gpointer user_data) {
    GtkWidget *container = GTK_WIDGET(user_data);
    gtk_widget_destroy(play_button);
    
    update_board_grid();
    
    button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_grid_attach(GTK_GRID(main_grid), button_box, 0, 1, 1, 1);
    btn_up = gtk_button_new_with_label("▲");
    btn_down = gtk_button_new_with_label("▼");
    btn_left = gtk_button_new_with_label("◀");
    btn_right = gtk_button_new_with_label("▶");
    btn_restart = gtk_button_new_with_label("Restart");
    gtk_box_pack_start(GTK_BOX(button_box), btn_up, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(button_box), btn_down, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(button_box), btn_left, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(button_box), btn_right, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(button_box), btn_restart, TRUE, TRUE, 0);
    g_signal_connect(btn_up, "clicked", G_CALLBACK(up_button_clicked), NULL);
    g_signal_connect(btn_down, "clicked", G_CALLBACK(down_button_clicked), NULL);
    g_signal_connect(btn_left, "clicked", G_CALLBACK(left_button_clicked), NULL);
    g_signal_connect(btn_right, "clicked", G_CALLBACK(right_button_clicked), NULL);
    g_signal_connect(btn_restart, "clicked", G_CALLBACK(restart_button_clicked), NULL);
    
    sprintf(msg_info, "%d items left!", count);
    label = gtk_label_new(msg_info);
    gtk_grid_attach(GTK_GRID(main_grid), label, 1, 0, 1, 1);
    
    gtk_widget_show_all(main_grid);
}

void on_key_press(GtkWidget *window, GdkEventKey *event, gpointer user_data){
    if (event->keyval == GDK_KEY_Up) {
        gtk_button_clicked(GTK_BUTTON(btn_up));
    }
    if (event->keyval == GDK_KEY_Down) {
        gtk_button_clicked(GTK_BUTTON(btn_down));
    }
    if (event->keyval == GDK_KEY_Left) {
        gtk_button_clicked(GTK_BUTTON(btn_left));
    }
    if (event->keyval == GDK_KEY_Right) {
        gtk_button_clicked(GTK_BUTTON(btn_right));
    }
}

void up_button_clicked(GtkWidget *up_button, gpointer user_data) {
    if(board[player.row-1][player.col]==BARRIER) return;
    else if(board[player.row-1][player.col]==GOAL) return;
    else if(board[player.row-1][player.col]==DIA){
        if(board[player.row-2][player.col]==0){
            board[player.row-2][player.col]=DIA;
            board[player.row-1][player.col]=PLAYER;
            board[player.row][player.col]=0;
            player.row = player.row-1;
            update_board_grid();
            gtk_widget_show_all(window);
        }
        else if(board[player.row-2][player.col]==GOAL){
            board[player.row][player.col+1]=PLAYER;
            board[player.row][player.col]=0;
            player.row = player.row+1;
            update_board_grid();
            count--;
            sprintf(msg_info, "%d items left!", count);
            gtk_label_set_text(GTK_LABEL(label), msg_info);
            gtk_widget_show_all(window);
        }
    }
    else{
        board[player.row-1][player.col] = PLAYER;
        board[player.row][player.col] = 0;
        player.row = player.row-1;
        update_board_grid();
        gtk_widget_show_all(window);
    }
}
void down_button_clicked(GtkWidget *down_button, gpointer user_data) {
    if(board[player.row+1][player.col]==BARRIER) return;
    else if(board[player.row+1][player.col]==GOAL) return;
    else if(board[player.row+1][player.col]==DIA){
        if(board[player.row+2][player.col]==0){
            board[player.row+2][player.col]=DIA;
            board[player.row+1][player.col]=PLAYER;
            board[player.row][player.col]=0;
            player.row = player.row+1;
            update_board_grid();
            gtk_widget_show_all(window);
        }
        else if(board[player.row+2][player.col]==GOAL){
            board[player.row+1][player.col]=PLAYER;
            board[player.row][player.col]=0;
            player.row = player.row+1;
            count--;
            sprintf(msg_info, "%d items left!", count);
            gtk_label_set_text(GTK_LABEL(label), msg_info);
            update_board_grid();
            gtk_widget_show_all(window);
        }
    }
    else{
        board[player.row+1][player.col] = PLAYER;
        board[player.row][player.col] = 0;
        player.row = player.row+1;
        update_board_grid();
        gtk_widget_show_all(window);
    }
}
void left_button_clicked(GtkWidget *left_button, gpointer user_data) {
    if(board[player.row][player.col-1]==BARRIER) return;
    else if(board[player.row][player.col-1]==GOAL) return;
    else if(board[player.row][player.col-1]==DIA){
        if(board[player.row][player.col-2]==0){
            board[player.row][player.col-2]=DIA;
            board[player.row][player.col-1]=PLAYER;
            board[player.row][player.col]=0;
            player.col = player.col-1;
            update_board_grid();
            gtk_widget_show_all(window);
        }
        else if(board[player.row][player.col-2]==GOAL){
            board[player.row][player.col-1]=PLAYER;
            board[player.row][player.col]=0;
            player.col = player.col-1;
            count--;
            sprintf(msg_info, "%d items left!", count);
            gtk_label_set_text(GTK_LABEL(label), msg_info);
            update_board_grid();
            gtk_widget_show_all(window);
        }
    }
    else{
        board[player.row][player.col-1] = PLAYER;
        board[player.row][player.col] = 0;
        player.col = player.col-1;
        update_board_grid();
        gtk_widget_show_all(window);
    }
}
void right_button_clicked(GtkWidget *right_button, gpointer user_data) {
    if(board[player.row][player.col+1]==BARRIER) return;
    else if(board[player.row][player.col+1]==GOAL) return;
    else if(board[player.row][player.col+1]==DIA){
        if(board[player.row][player.col+2]==0){
            board[player.row][player.col+2]=DIA;
            board[player.row][player.col+1]=PLAYER;
            board[player.row][player.col]=0;
            player.col = player.col+1;
            update_board_grid();
            gtk_widget_show_all(window);
        }
        else if(board[player.row][player.col+2]==GOAL){
            board[player.row][player.col+1]=PLAYER;
            board[player.row][player.col]=0;
            player.col = player.col+1;
            count--;
            sprintf(msg_info, "%d items left!", count);
            gtk_label_set_text(GTK_LABEL(label), msg_info);
            update_board_grid();
            gtk_widget_show_all(window);
        }
    }
    else{
        board[player.row][player.col+1] = PLAYER;
        board[player.row][player.col] = 0;
        player.col = player.col+1;
        sprintf(msg_info, "%d items left!", count);
        gtk_label_set_text(GTK_LABEL(label), msg_info);
        update_board_grid();
        gtk_widget_show_all(window);
    }
}
void restart_button_clicked(GtkWidget *restart_button, gpointer user_data) {
    init_board();
    sprintf(msg_info, "%d items left!", count);
    gtk_label_set_text(GTK_LABEL(label), msg_info);
    update_board_grid();
    gtk_widget_show_all(window);
}


int main(int argc, char *argv[]) {

    //initialize board
    init_board();
    gtk_init(&argc, &argv);

    //window setting
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "PushPush V1");
    gtk_window_set_default_size(GTK_WINDOW(window), 500, 400);
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(window, "key-press-event", G_CALLBACK(on_key_press), NULL);

    //main grid
    main_grid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(window), main_grid);

    //make play button
    play_button = gtk_button_new_with_label("Play");
    g_signal_connect(play_button, "clicked", G_CALLBACK(play_button_clicked), main_grid);
    gtk_widget_set_size_request(play_button, 100, 50);
    gtk_grid_attach(GTK_GRID(main_grid), play_button, 0, 0, 1, 1);

    gtk_widget_show_all(window);
    gtk_widget_grab_focus(window);
    gtk_main();

    return 0;
}
