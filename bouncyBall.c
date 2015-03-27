#include <ncurses.h>
#include <unistd.h>
#include <math.h>
#define uSEC_PER_SEC 1000000
#define DELAY 25000
#define FIRE_SPEED 4
#define g -9.81
#define COLLISION_ELASTICITY 0.7
#define DRAG_COEFFICIENT 0.8
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
  bool needs_restart = false;
  switch(c){
  case ARROW_KEY_UP:
    {
      ball->upVelocity += FIRE_SPEED;
      needs_restart = true;
      break;
    }
  case ARROW_KEY_RIGHT:
    {
      ball->rightVelocity += FIRE_SPEED;
      needs_restart = true;
      break;
    }
  case ARROW_KEY_LEFT:
    {
      ball->rightVelocity -= FIRE_SPEED;
      needs_restart = true;
      break;
    }
  case ARROW_KEY_DOWN:
    ball->upVelocity -= FIRE_SPEED;
    needs_restart = true;
  default:
    break;
  }
  return needs_restart;
}


void mainloop(Ball* ball, int lines, int cols, float acc)
{
  bool needs_restart = false;
  do {
    /* Deal with the ball in projectile motion. */
    while(ball->y > 1 || ball->upVelocity > 0.1 || ball->upVelocity < -0.1)
      {
	handle_keypress(getch(), ball);
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
	usleep(DELAY);
      }
    // Ensure there's no lingering momentum we ignored as being too small earlier.
    ball->upVelocity = 0;

    /* Deal with the ball rolling on the floor against friction. */
    while (ball->rightVelocity >= 0.5 || ball->rightVelocity <= -0.5)
      {
	int c;
	while ((c = getch()) != ERR)
	  {
	    if(handle_keypress(c, ball))
	      {
		continue;
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
      }
    ball->rightVelocity = 0;
  } while (needs_restart);
  // Ensure there's no lingering momentum we ignored as being too small earlier.
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
  initscr();
  noecho();
  curs_set(FALSE);
  nodelay(stdscr, TRUE);

  int lines, cols;
  getmaxyx(stdscr, lines, cols);
  float acc = g / (float)lines;

  Ball ball;
  ball.x = 5;
  ball.y = lines;
  ball.upVelocity = 0;
  ball.rightVelocity = 1;
  
  mainloop(&ball, lines, cols, acc);
  display_instructions();
  int c;
  while ((c = getch()) != 'q')
    {
      if (c == ERR)
	continue;
      handle_keypress(c, &ball);
      mainloop(&ball, lines, cols, acc);
      display_instructions();
    }
  endwin();
}
