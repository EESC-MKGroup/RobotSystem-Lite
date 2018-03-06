////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//  Copyright (c) 2016-2018 Leonardo Consoni <consoni_2519@hotmail.com>       //
//                                                                            //
//  This file is part of RobotSystem-Lite.                                    //
//                                                                            //
//  RobotSystem-Lite is free software: you can redistribute it and/or modify  //
//  it under the terms of the GNU Lesser General Public License as published  //
//  by the Free Software Foundation, either version 3 of the License, or      //
//  (at your option) any later version.                                       //
//                                                                            //
//  RobotSystem-Lite is distributed in the hope that it will be useful,       //
//  but WITHOUT ANY WARRANTY; without even the implied warranty of            //
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the              //
//  GNU Lesser General Public License for more details.                       //
//                                                                            //
//  You should have received a copy of the GNU Lesser General Public License  //
//  along with RobotSystem-Lite. If not, see <http://www.gnu.org/licenses/>.  //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////


#include "motors.h"

#include "signal_io/signal_io.h"

//#include "debug/async_debug.h"
      
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
      
struct _MotorData
{
  DECLARE_MODULE_INTERFACE_REF( SIGNAL_IO_INTERFACE );
  int interfaceID;
  unsigned int outputChannel;
  double outputGain;
  bool isEnabled;
};


Motor Motor_Init( DataHandle configuration )
{
  static char filePath[ DATA_IO_MAX_PATH_LENGTH ];
  
  if( configuration == NULL ) return NULL;
  
  const char* motorName = DataIO_GetStringValue( configuration, NULL, "" );
  if( motorName != NULL )
  {
    sprintf( filePath, MOTORS_CONFIG_PATH "/%s", motorName );
    if( (configuration = DataIO_LoadStorageData( filePath )) == NULL ) return NULL;
  }
  
  Motor newMotor = (Motor) malloc( sizeof(MotorData) );
  memset( newMotor, 0, sizeof(MotorData) );

  bool loadSuccess = true;
  sprintf( filePath, SIGNAL_IO_MODULES_PATH "/%s", DataIO_GetStringValue( configuration, "", "output_interface.type" ) );
  LOAD_MODULE_IMPLEMENTATION( SIGNAL_IO_INTERFACE, filePath, newMotor, &loadSuccess );
  if( loadSuccess )
  {
    newMotor->interfaceID = newMotor->InitDevice( DataIO_GetStringValue( configuration, "", "output_interface.config" ) );
    if( newMotor->interfaceID != SIGNAL_IO_DEVICE_INVALID_ID ) 
    {
      newMotor->outputChannel = (unsigned int) DataIO_GetNumericValue( configuration, -1, "output_interface.channel" );
      //DEBUG_PRINT( "trying to aquire channel %u from interface %d", newMotor->outputChannel, newMotor->interfaceID );
      loadSuccess = newMotor->AcquireOutputChannel( newMotor->interfaceID, newMotor->outputChannel );
    }
    else loadSuccess = false;
  }
  
  newMotor->outputGain = DataIO_GetNumericValue( configuration, 1.0, "output_gain.multiplier" );
  newMotor->outputGain /= DataIO_GetNumericValue( configuration, 1.0, "output_gain.divisor" );
  
  newMotor->isEnabled = false;
  
  if( motorName != NULL ) DataIO_UnloadData( configuration );
  
  if( !loadSuccess )
  {
    Motor_End( newMotor );
    return NULL;
  }
  
  return newMotor;
}

void Motor_End( Motor motor )
{
  if( motor == NULL ) return;
  
  motor->EndDevice( motor->interfaceID );
  
  free( motor );
}

bool Motor_Enable( Motor motor )
{
  if( motor == NULL ) return false;
  
  motor->Reset( motor->interfaceID );
  return motor->AcquireOutputChannel( motor->interfaceID, motor->outputChannel );
}

void Motor_Disable( Motor motor )
{
  if( motor == NULL ) return;
  
  motor->ReleaseOutputChannel( motor->interfaceID, motor->outputChannel );
}

void Motor_Reset( Motor motor )
{
  if( motor == NULL ) return;
  
  motor->Reset( motor->interfaceID );
}

bool Motor_HasError( Motor motor )
{
  if( motor == NULL ) return false;
  
  return motor->HasError( motor->interfaceID );
}

void Motor_WriteControl( Motor motor, double setpoint )
{
  if( motor == NULL ) return;
  
  //if( setpoint != 0.0 ) DEBUG_PRINT( "output: %.5f * %.5f = %.5f", motor->outputGain, setpoint, setpoint * motor->outputGain );
  
  setpoint = setpoint * motor->outputGain;
  
  motor->Write( motor->interfaceID, motor->outputChannel, setpoint );
}
