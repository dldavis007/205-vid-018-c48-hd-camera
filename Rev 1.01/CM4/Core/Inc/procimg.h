/**************************************************************************
MODULE:    PROCIMG
CONTAINS:  Process Image Configuration
COPYRIGHT: Embedded Systems Academy, Inc. 2002-2005.
           All rights reserved. www.microcanopen.com
           This software was written in accordance to the guidelines at
           www.esacademy.com/software/softwarestyleguide.pdf
DISCLAIM:  Read and understand our disclaimer before using this code!
           www.esacademy.com/disclaim.htm
LICENSE:   THIS IS THE EDUCATIONAL VERSION OF MICROCANOPEN
           See file license_educational.txt or
           www.microcanopen.com/license_educational.txt
           A commercial MicroCANopen license is available at
           www.CANopenStore.com
VERSION:   2.10, ESA 12-JAN-05
           $LastChangedDate: 2005-01-12 13:53:59 -0700 (Wed, 12 Jan 2005) $
           $LastChangedRevision: 48 $
***************************************************************************/ 

#ifndef _PROCIMG_H
#define _PROCIMG_H


/**************************************************************************
DEFINES: Definition of the process image
Modify these for your application
**************************************************************************/


// Define the size of the process image
#define PROCIMG_SIZE 24

// Define process variables: offsets into the process image 
// menu control
#define IN_digi_0   0

// camera address lsb, used to transmit camera address
#define IN_digi_1   1
// camera address msb
#define IN_digi_2   2

//IN_digi_ 3 thru 10 store the 8 byte camera menu string
#define IN_digi_3   3   
#define IN_digi_4   4
#define IN_digi_5   5
#define IN_digi_6   6
#define IN_digi_7   7
#define IN_digi_8   8
#define IN_digi_9   9
#define IN_digi_10   10

// menu control
#define OUT_digi_0  11
// button box data 3 bytes
#define OUT_digi_1  12
#define OUT_digi_2  13
#define OUT_digi_3  14
// incoming/active camera address lsb
#define OUT_digi_4  15
// incoming/active camera address msb
#define OUT_digi_5  16
// command byte
#define OUT_digi_6  17

// camera address to match lsb
#define OUT_digi_7  18
// camera address to match msb
#define OUT_digi_8  19

// new assigned camera address lsb
#define OUT_digi_9  20
// new assigned camera address msb
#define OUT_digi_10  21
#define OUT_digi_11  22







#endif
