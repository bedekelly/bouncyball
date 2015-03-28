#include <ncurses.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#define uSEC_PER_SEC 1000000
#define DELAY 25000
// 25000
#define FIRE_SPEED 4
#define g -9.81
#define COLLISION_ELASTICITY 0.7
#define DRAG_COEFFICIENT 0.8
#define MIN_LINES_PER_DELAY 1
#define MIN_COLS_PER_DELAY 0.3
#define ON_FLOOR 1
#define ARROW_KEY_UP 'w'
#define ARROW_KEY_DOWN 's'
#define ARROW_KEY_LEFT 'a'
#define ARROW_KEY_RIGHT 'd'

typedef struct
{
  float x;
  float y;
  float upVelocity; // Taking 'up' as +ve
  float rightVelocity; // Taking 'right' as +ve
} Ball;


bool handle_keypress(int c, Ball* ball)
{
  bool should_quit = false;
  switch(c){
  case ARROW_KEY_UP:
    {
      ball->upVelocity += FIRE_SPEED;
      break;
    }
  case ARROW_KEY_RIGHT:
    {
      ball->rightVelocity += FIRE_SPEED;
      break;
    }
  case ARROW_KEY_LEFT:
    {
      ball->rightVelocity -= FIRE_SPEED;
      break;
    }
  case ARROW_KEY_DOWN:
    ball->upVelocity -= FIRE_SPEED;
  case 'q':
    should_quit = true;
  default:
    break;
  }
  return should_quit;
}


int mainloop(Ball* ball, int lines, int cols, float acc, bool debug)
{
  FILE *fp;
  if (debug)
    {
      fp = fopen("debug", "a");
    }

  /* Deal with the ball in projectile motion. */
  while(ball->y > ON_FLOOR || (ball->upVelocity > MIN_LINES_PER_DELAY
			       || ball->upVelocity < -MIN_LINES_PER_DELAY))
    {
      if(handle_keypress(getch(), ball) && debug)
	{
	  return 1;
	}
      clear();
      /* Handle collision with the floor */
      if (ball->y + ball->upVelocity <= 0)
	{
	  usleep(DELAY);
	  ball->y = 0;
	  ball->upVelocity *= (-1 * COLLISION_ELASTICITY);
	}

      /* Handle collision with the walls */
      if (ball->x + ball->rightVelocity >= cols-1)
	{
	  usleep(DELAY);
	  ball->x = cols;
	  ball->rightVelocity *= (-1 * COLLISION_ELASTICITY);
	}
      else if (ball->x + ball->rightVelocity <= 0)
	{
	  usleep(DELAY);
	  ball->x = 0;
	  ball->rightVelocity *= (-1 * COLLISION_ELASTICITY);
	}

      /* Update the ball's velocity and print accordingly. */
      ball->y += ball->upVelocity;
      ball->x += ball->rightVelocity;
      ball->upVelocity += acc;
      mvprintw(lines-ball->y, ball->x, "o");
      refresh();
      if (debug){
	fprintf(fp, "\n\nUp velocity: %2.2f", ball->upVelocity);
	fprintf(fp, "\nRight velocity: %2.2f", ball->rightVelocity);
	fflush(fp);
      }
      usleep(DELAY);
    }
  // Ensure there's no lingering momentum we ignored as being too small earlier.
  ball->upVelocity = 0;

  /* Deal with the ball rolling on the floor against friction. */
  while (ball->rightVelocity >= MIN_COLS_PER_DELAY
	 || ball->rightVelocity <= -MIN_COLS_PER_DELAY)
    {
      int c;
      while ((c = getch()) != ERR)
	{
	  if(handle_keypress(c, ball) && debug)
	    {
	      return 1;
	    }
	}
      usleep(DELAY);

      /* Handle collisions with the right and left walls. */
      if (ball->x + ball->rightVelocity >= cols-1)
	{
	  usleep(DELAY);
	  ball->x = cols;
	  ball->rightVelocity *= (-1 * COLLISION_ELASTICITY);
	}
      else if (ball->x + ball->rightVelocity <= 0)
	{
	  usleep(DELAY);
	  ball->x = 0;
	  ball->rightVelocity *= (-1 * COLLISION_ELASTICITY);
	}

      /* Update the ball's velocity and print accordingly. */
      ball->rightVelocity *= DRAG_COEFFICIENT;
      ball->x += ball->rightVelocity;
      clear();
      mvprintw(lines-ball->y, ball->x, "o");
      refresh();
      if (debug)
	{
	  fprintf(fp, "\n\nUp velocity: %2.2f", ball->upVelocity);
	  fprintf(fp, "\nRight velocity: %2.2f", ball->rightVelocity);
	  fflush(fp);
	}
    }
  ball->rightVelocity = 0;

  // Ensure there's no lingering momentum we ignored as being too small earlier.
  ball->upVelocity = 0;
  ball->rightVelocity = 0;
  if(debug)
    {
      fprintf(fp, "\n\nUp velocity: %2.2f", ball->upVelocity);
      fprintf(fp, "\nRight velocity: %2.2f", ball->rightVelocity);
      fflush(fp);
      fclose(fp);
    }
}


void display_instructions()
{
  mvprintw(4, 3, "W: Fire upwards");
  mvprintw(5, 3, "A: Fire left");
  mvprintw(6, 3, "D: Fire right");
  mvprintw(7, 3, "Q: Quit");
  WINDOW *instructions;
  int width = 20, height = 6;
  int x_offset=1, y_offset=3;
  instructions = newwin(height, width, y_offset, x_offset);
  box(instructions, 0, 0);
  wrefresh(instructions);
  refresh();
}


int main(int argc, char* argv[])
{
  printf("%d", argc);
  initscr();
  noecho();
  curs_set(FALSE);
  nodelay(stdscr, TRUE);

  bool debug = false;

  if (argc == 2)
    {
      debug = (!strcmp(argv[1], "--debug"));
    }
  if (debug)
    {
      mvprintw(4, 4, "DEBUG MODE");
      mvprintw(5, 4, "To see debug output, 'tail -f debug' in this directory.");
      mvprintw(6, 4, "Press any key to begin.");
      refresh();
      getchar();
    }

  int lines, cols;
  getmaxyx(stdscr, lines, cols);
  float acc = g / (float)lines;

  Ball ball;
  ball.x = 5;
  ball.y = lines;
  ball.upVelocity = 0;
  ball.rightVelocity = 1;
  
  mainloop(&ball, lines, cols, acc, debug);
  display_instructions();
  int c;
  while ((c = getch()) != 'q')
    {
      if (c == ERR)
	continue;
      handle_keypress(c, &ball);
      mainloop(&ball, lines, cols, acc, debug);
      display_instructions();
    }
  endwin();
}
