#include <ncurses.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#define uSEC_PER_SEC 1000000
#define DELAY 25000
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
  /*
    Take the keypress data as input and determine how to alter the ball's
     velocity.If the key is 'q', return true to let the mainloop know the user
     wants to quit.
  */

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
    {
      ball->upVelocity -= FIRE_SPEED;
      break;
    }
  case 'q':
    {
      should_quit = true;
      break;
    }
  }
  return should_quit;
}


int handle_motion(Ball* ball, int lines, int cols, float acc, bool debug)
{
  // If we're in debugging mode, open a file to which we can write messages.
  FILE *fp;
  if (debug)
    {
      fp = fopen("debug", "a");
    }

  /*
    Deal with the ball in projectile motion. 
    Loop condition: Either the ball isn't on the floor, or it's moving fast enough
    towards/away from the floor that we know it'll bounce upwards again.
   */
  while(ball->y > ON_FLOOR || (ball->upVelocity > MIN_LINES_PER_DELAY
			       || ball->upVelocity < -MIN_LINES_PER_DELAY))
    {
      usleep(DELAY);

      /* If there's a character in the input buffer, read it and alter the ball's
	 velocity accordingly. */
      int c;
      while((c = getch()) != ERR)
	{
	  if(handle_keypress(c, ball))
	    {
	      return 1;  // Exit point when the user presses Q.
	    }
	}

      /* Handle collision with the floor. */
      if (ball->y + ball->upVelocity <= 0)
	{
	  usleep(DELAY);
	  ball->y = 0;
	  ball->upVelocity *= (-1 * COLLISION_ELASTICITY);
	}

      /* Handle collision with the right wall. */
      if (ball->x + ball->rightVelocity >= cols-1)
	{
	  usleep(DELAY);
	  ball->x = cols;
	  ball->rightVelocity *= (-1 * COLLISION_ELASTICITY);
	}
      
      /* Handle collision with the left wall. */
      else if (ball->x + ball->rightVelocity <= 0)
	{
	  usleep(DELAY);
	  ball->x = 0;
	  ball->rightVelocity *= (-1 * COLLISION_ELASTICITY);
	}

      /* Update the ball's position due to velocity, and then velocity due to
	 acceleration. */
      ball->y += ball->upVelocity;
      ball->x += ball->rightVelocity;
      ball->upVelocity += acc;
      
      /* Update the ball's position on the screen accordingly. */
      clear();
      mvprintw(lines-ball->y, ball->x, "o");
      refresh();

      // If we're in debug mode, output the ball's velocity components.
      if (debug){
	fprintf(fp, "\n\nUp velocity: %2.2f", ball->upVelocity);
	fprintf(fp, "\nRight velocity: %2.2f", ball->rightVelocity);
	fflush(fp);  // Ensure we're actually writing the data NOW.
      }
    }
  
  /* Deal with the ball rolling on the floor against friction. */
  while (ball->rightVelocity >= MIN_COLS_PER_DELAY
	 || ball->rightVelocity <= -MIN_COLS_PER_DELAY)
    {
      usleep(DELAY);

      int c;
      while((c = getch()) != ERR)
	{
	  if(handle_keypress(c, ball))
	    {
	      return 1;  // Exit point when the user presses Q.
	    }
	}
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

      /* Update the ball's velocity due to drag. */
      ball->rightVelocity *= DRAG_COEFFICIENT;
      ball->x += ball->rightVelocity;

      /* Update the ball's position on screen accordingly. */
      clear();
      mvprintw(lines-ball->y, ball->x, "o");
      refresh();
      
      /* If we're in debug mode, write the ball's velocity components to the
	 debug file. */
      if (debug)
	{
	  fprintf(fp, "\n\nUp velocity: %2.2f", ball->upVelocity);
	  fprintf(fp, "\nRight velocity: %2.2f", ball->rightVelocity);
	  fflush(fp);  // Ensure we're actually writing the data NOW.
	}
    }

  // Ensure there's no lingering momentum we ignored as being too small earlier.
  ball->upVelocity = 0;
  ball->rightVelocity = 0;

  /* If we're in debug mode, write the ball's velocity components to the
     debug file. */
  if(debug)
    {
      fprintf(fp, "\n\nUp velocity: %2.2f", ball->upVelocity);
      fprintf(fp, "\nRight velocity: %2.2f", ball->rightVelocity);
      fflush(fp);  // Ensure we're writing the data NOW.
      fclose(fp);  // Close our handle to the file buffer.
    }
}


void display_instructions()
{
  // Print out usage instructions to the top left-hand side of the screen.
  mvprintw(2, 4, "W: Fire upwards");
  mvprintw(3, 4, "A: Fire left");
  mvprintw(4, 4, "D: Fire right");
  mvprintw(5, 4, "S: Fire downwards");
  mvprintw(6, 4, "Q: Quit");

  // Outline the instructions by making a subwindow and 'box'ing it.
  WINDOW *instructions;
  int width = 21, height = 7;
  int x_offset=2, y_offset=1;
  instructions = newwin(height, width, y_offset, x_offset);
  box(instructions, 0, 0);

  // Refresh the boxed window, and lastly the whole screen.
  wrefresh(instructions);
  refresh();
}


int main(int argc, char* argv[])
{
  // Setup curses.
  initscr();
  noecho();
  curs_set(FALSE);

  // Make sure we don't have a delay after character presses.
  nodelay(stdscr, TRUE);

  // Check for the --debug argument.
  bool debug = false;
  if (argc == 2)
    {
      debug = (!strcmp(argv[1], "--debug"));
    }
  if (debug)
    {
      // If we're in debug mode, let the user know how to see output.
      mvprintw(4, 4, "DEBUG MODE");
      mvprintw(5, 4, "To see debug output, 'tail -f debug' in this directory.");
      mvprintw(6, 4, "Press any key to begin.");
      refresh();
      getchar();
    }

  // Get the terminal size, and set the acceleration accordingly.
  int lines, cols;
  getmaxyx(stdscr, lines, cols);
  float acc = g / (float)lines;

  // Make a new ball, and give it some initial rightward velocity.
  Ball ball;
  ball.x = 5;
  ball.y = lines;
  ball.upVelocity = 0;
  ball.rightVelocity = 1;
  
  // If the user hasn't quit within the first bounce:
  if(!handle_motion(&ball, lines, cols, acc, debug))
    {
      // Display usage instructions for the first time.
      display_instructions();

      // Loop until the user presses Q.
      int c;
      while ((c = getch()) != 'q') {
	if (c == ERR)  // If the user hasn't pressed anything yet, continue.
	  continue;
	
	// Handle the key pressed by the user. If it's Q, exit the program.
	if (handle_keypress(c, &ball)) break;
	
	/* Deal with the ball's subsequent motion. If the user presses Q at any
	point, exit the program. */
	if (handle_motion(&ball, lines, cols, acc, debug)) break;

	/* User hasn't pressed Q and the ball's come to rest: display instructions
	   again. */
	display_instructions();
      }
    }

  // Return terminal control to its previous settings and exit the program.
  endwin();
}
