///////////////////////////////////////////////////////////////////////////
//                                                                       //
// steppers.c steppermotor emulation                                     //
//                                                                       //
// Emulates : 48 step motors driven with full step or half step          //
//            also emulates the index optic                              //
//                                                                       //
// 05-03-2004: Re-Animator                                               //
//                                                                       //
// TODO: ?add functions to set index stuff                               //
//        add different types of stepper motors if needed                //
//                                                                       //
//       - keep track of the rotation speed, useful for motion blur      //
//       - pinmame mech.c uses a clever trick involving mech pulses per	 //
//         VBLANK, worth including here?                                 //
// Any fixes for this driver should be forwarded to AGEMAME HQ           //
// (http://www.mameworld.net/agemame/)                                   //
///////////////////////////////////////////////////////////////////////////

#include "driver.h"
#include "steppers.h"

// local prototypes ///////////////////////////////////////////////////////

static void update_optic(int id);

// local vars /////////////////////////////////////////////////////////////

static struct
{
  UINT8 pattern,	  // coil pattern
	    old_pattern;  // old coil pattern
	    
  INT16	step_pos,	  // step position 0 - max_steps
	    max_steps;	  // maximum step position
		
  INT16	index_pos,	  // position of index (in steps)
	    index_len,	  // length of index   (in steps)
		index_patt;	  // pattern needed on coils (0=don't care)

  UINT8	optic;

} steppers[MAX_STEPPERS];


// step table, use previouspattern::newpattern as index
//
static const int StepTab[] =
{
   0, //0000->0000 0->0
   0, //0000->0001 0->1
   0, //0000->0010 0->2
   0, //0000->0011 0->3
   0, //0000->0100 0->4
   0, //0000->0101 0->5
   0, //0000->0110 0->6
   0, //0000->0111 0->7
   0, //0000->1000 0->8
   0, //0000->1001 0->9
   0, //0000->1010 0->A
   0, //0000->1011 0->B
   0, //0000->1100 0->C
   0, //0000->1101 0->D
   0, //0000->1110 0->E
   0, //0000->1111 0->F

   0, //0001->0000 1->0
   0, //0001->0001 1->1
   0, //0001->0010 1->2
   0, //0001->0011 1->3
  -2, //0001->0100 1->4
  -1, //0001->0101 1->5
   0, //0001->0110 1->6
   0, //0001->0111 1->7
   2, //0001->1000 1->8
   1, //0001->1001 1->9
   0, //0001->1010 1->A
   0, //0001->1011 1->B
   0, //0001->1100 1->C
   0, //0001->1101 1->D
   0, //0001->1110 1->E
   0, //0001->1111 1->F

   0, //0010->0000 2->0
   0, //0010->0001 2->1
   0, //0010->0010 2->2
   0, //0010->0011 2->3
   2, //0010->0100 2->4
   0, //0010->0101 2->5
   1, //0010->0110 2->6
   0, //0010->0111 2->7
  -2, //0010->1000 2->8
   0, //0010->1001 2->9
  -1, //0010->1010 2->A
   0, //0010->1011 2->B
   0, //0010->1100 2->C
   0, //0010->1101 2->D
   0, //0010->1110 2->E
   0, //0010->1111 2->F

   0, //0011->0000 3->0
   0, //0011->0001 3->1
   0, //0011->0010 3->2
   0, //0011->0011 3->3
   0, //0011->0100 3->4
   0, //0011->0101 3->5
   0, //0011->0110 3->6
   0, //0011->0111 3->7
   0, //0011->1000 3->8
   0, //0011->1001 3->9
   0, //0011->1010 3->A
   0, //0011->1011 3->B
   0, //0011->1100 3->C
   0, //0011->1101 3->D
   0, //0011->1110 3->E
   0, //0011->1111 3->F

   0, //0100->0000 4->0
   2, //0100->0001 4->1
  -2, //0100->0010 4->2
   0, //0100->0011 4->3
   0, //0100->0100 4->4
   1, //0100->0101 4->5
  -1, //0100->0110 4->6
   0, //0100->0111 4->7
   0, //0100->1000 4->8
   0, //0100->1001 4->9
   0, //0100->1010 4->A
   0, //0100->1011 4->B
   0, //0100->1100 4->C
   0, //0100->1101 4->D
   0, //0100->1110 4->E
   0, //0100->1111 4->F

   0, //0101->0000 5->0
   1, //0101->0001 5->1
   0, //0101->0010 5->2
   0, //0101->0011 5->3
  -1, //0101->0100 5->4
   0, //0101->0101 5->5
  -2, //0101->0110 5->6
   0, //0101->0111 5->7
   0, //0101->1000 5->8
   2, //0101->1001 5->9
   0, //0101->1010 5->A
   0, //0101->1011 5->B
   0, //0101->1100 5->C
   0, //0101->1101 5->D
   0, //0101->1110 5->E
   0, //0101->1111 5->F

   0, //0110->0000 6->0
   0, //0110->0001 6->1
  -1, //0110->0010 6->2
   0, //0110->0011 6->3
   1, //0110->0100 6->4
   2, //0110->0101 6->5
   0, //0110->0110 6->6
   0, //0110->0111 6->7
   0, //0110->1000 6->8
   0, //0110->1001 6->9
  -2, //0110->1010 6->A
   0, //0110->1011 6->B
   0, //0110->1100 6->C
   0, //0110->1101 6->D
   0, //0110->1110 6->E
   0, //0110->1111 6->F

   0, //0111->0000 7->0
   0, //0111->0001 7->1
   0, //0111->0010 7->2
   0, //0111->0011 7->3
   0, //0111->0100 7->4
   0, //0111->0101 7->5
   0, //0111->0110 7->6
   0, //0111->0111 7->7
   0, //0111->1000 7->8
   0, //0111->1001 7->9
   0, //0111->1010 7->A
   0, //0111->1011 7->B
   0, //0111->1100 7->C
   0, //0111->1101 7->D
   0, //0111->1110 7->E
   0, //0111->1111 7->F

   0, //1000->0000 8->0
  -2, //1000->0001 8->1
   2, //1000->0010 8->2
   0, //1000->0011 8->3
   0, //1000->0100 8->4
   0, //1000->0101 8->5
   0, //1000->0110 8->6
   0, //1000->0111 8->7
   0, //1000->1000 8->8
  -1, //1000->1001 8->9
   1, //1000->1010 8->A
   0, //1000->1011 8->B
   0, //1000->1100 8->C
   0, //1000->1101 8->D
   0, //1000->1110 8->E
   0, //1000->1111 8->F

   0, //1001->0000 9->0
  -1, //1001->0001 9->1
   0, //1001->0010 9->2
   0, //1001->0011 9->3
   0, //1001->0100 9->4
  -2, //1001->0101 9->5
   0, //1001->0110 9->6
   0, //1001->0111 9->7
   1, //1001->1000 9->8
   0, //1001->1001 9->9
   2, //1001->1010 9->A
   0, //1001->1011 9->B
   0, //1001->1100 9->C
   0, //1001->1101 9->D
   0, //1001->1110 9->E
   0, //1001->1111 9->F

   0, //1010->0000 A->0
   0, //1010->0001 A->1
   1, //1010->0010 A->2
   0, //1010->0011 A->3
   0, //1010->0100 A->4
   0, //1010->0101 A->5
   2, //1010->0110 A->6
   0, //1010->0111 A->7
  -1, //1010->1000 A->8
  -2, //1010->1001 A->9
   0, //1010->1010 A->A
   0, //1010->1011 A->B
   0, //1010->1100 A->C
   0, //1010->1101 A->D
   0, //1010->1110 A->E
   0, //1010->1111 A->F

   0, //1011->0000 B->0
   0, //1011->0001 B->1
   0, //1011->0010 B->2
   0, //1011->0011 B->3
   0, //1011->0100 B->4
   0, //1011->0101 B->5
   0, //1011->0110 B->6
   0, //1011->0111 B->7
   0, //1011->1000 B->8
   0, //1011->1001 B->9
   0, //1011->1010 B->A
   0, //1011->1011 B->B
   0, //1011->1100 B->C
   0, //1011->1101 B->D
   0, //1011->1110 B->E
   0, //1011->1111 B->F

   0, //1100->0000 C->0
   0, //1100->0001 C->1
   0, //1100->0010 C->2
   0, //1100->0011 C->3
   0, //1100->0100 C->4
   0, //1100->0101 C->5
   0, //1100->0110 C->6
   0, //1100->0111 C->7
   0, //1100->1000 C->8
   0, //1100->1001 C->9
   0, //1100->1010 C->A
   0, //1100->1011 C->B
   0, //1100->1100 C->C
   0, //1100->1101 C->D
   0, //1100->1110 C->E
   0, //1100->1111 C->F

   0, //1101->0000 D->0
   0, //1101->0001 D->1
   0, //1101->0010 D->2
   0, //1101->0011 D->3
   0, //1101->0100 D->4
   0, //1101->0101 D->5
   0, //1101->0110 D->6
   0, //1101->0111 D->7
   0, //1101->1000 D->8
   0, //1101->1001 D->9
   0, //1101->1010 D->A
   0, //1101->1011 D->B
   0, //1101->1100 D->C
   0, //1101->1101 D->D
   0, //1101->1110 D->E
   0, //1101->1111 D->F

   0, //1110->0000 E->0
   0, //1110->0001 E->1
   0, //1110->0010 E->2
   0, //1110->0011 E->3
   0, //1110->0100 E->4
   0, //1110->0101 E->5
   0, //1110->0110 E->6
   0, //1110->0111 E->7
   0, //1110->1000 E->8
   0, //1110->1001 E->9
   0, //1110->1010 E->A
   0, //1110->1011 E->B
   0, //1110->1100 E->C
   0, //1110->1101 E->D
   0, //1110->1110 E->E
   0, //1110->1111 E->F

   0, //1111->0000 F->0
   0, //1111->0001 F->1
   0, //1111->0010 F->2
   0, //1111->0011 F->3
   0, //1111->0100 F->4
   0, //1111->0101 F->5
   0, //1111->0110 F->6
   0, //1111->0111 F->7
   0, //1111->1000 F->8
   0, //1111->1001 F->9
   0, //1111->1010 F->A
   0, //1111->1011 F->B
   0, //1111->1100 F->C
   0, //1111->1101 F->D
   0, //1111->1110 F->E
   0  //1111->1111 F->F
};

///////////////////////////////////////////////////////////////////////////

void Stepper_init(int id, int type)
{
  if ( id < MAX_STEPPERS )
  {
	steppers[id].index_pos   = 16;
	steppers[id].index_len   = 8;
	steppers[id].index_patt  = 0x09;
	steppers[id].pattern     = 0;
	steppers[id].old_pattern = 0;
	steppers[id].step_pos    = 0;
	steppers[id].max_steps   = 48*2;

	switch ( type )
	{
	case STEPPER_48STEP_REEL :	  // STARPOINT RMxxx

				break;

	case STEPPER_144STEPS_DICE :  // STARPOINT 1DCU DICE mechanism

				steppers[id].max_steps = 144*2;
				break;
	}
  }
}

///////////////////////////////////////////////////////////////////////////

int Stepper_get_position(int id)
{
  return steppers[id].step_pos;
}

///////////////////////////////////////////////////////////////////////////

int Stepper_get_max(int id)
{
  return steppers[id].max_steps;
}

///////////////////////////////////////////////////////////////////////////

static void update_optic(int id)
{
  int pos   = steppers[id].step_pos,
      index = steppers[id].index_pos;

  if ( ( pos >= index ) && ( pos <  index + steppers[id].index_len ) &&
	   ( ( steppers[id].pattern == steppers[id].index_patt || steppers[id].index_patt==0) ||
		 ( steppers[id].pattern == 0 &&
		   (steppers[id].old_pattern == steppers[id].index_patt || steppers[id].index_patt==0)
		 ) 
	   )
	 )
  {
	steppers[id].optic = 1;
  }
  else steppers[id].optic = 0;
}

///////////////////////////////////////////////////////////////////////////

void Stepper_reset_position(int id)
{
  steppers[id].step_pos    = 0;
  steppers[id].pattern     = 0x00;
  steppers[id].old_pattern = 0x00;

  update_optic(id);
}

///////////////////////////////////////////////////////////////////////////

int Stepper_optic_state(int id)
{
  int result = 0;

  if ( id < MAX_STEPPERS )
  {
	result = steppers[id].optic;
  }

  return result;
}

///////////////////////////////////////////////////////////////////////////

int Stepper_update(int id, UINT8 pattern)
{
  int changed = 0;

  pattern &= 0x0F;

  if ( steppers[id].pattern != pattern )
  { // pattern changed
	int index,
		steps,
		pos;

	if ( steppers[id].pattern )
	{
	  steppers[id].old_pattern = steppers[id].pattern;
	}
	steppers[id].pattern = pattern;

	index = (steppers[id].old_pattern << 4) | pattern;
	steps = StepTab[ index ];

	if ( steps )
	{
	  pos = steppers[id].step_pos + steps;

	  if ( pos > steppers[id].max_steps ) pos -= steppers[id].max_steps;
	  else if ( pos < 0 )                 pos += steppers[id].max_steps;

	  steppers[id].step_pos = pos;
	  update_optic(id);

	  changed++;
	  //logerror("reel %d: %3d %d\n", id, pos, steppers[id].optic);
	}
  }

  return changed;
}

