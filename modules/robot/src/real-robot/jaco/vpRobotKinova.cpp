/****************************************************************************
 *
 * ViSP, open source Visual Servoing Platform software.
 * Copyright (C) 2005 - 2019 by Inria. All rights reserved.
 *
 * This software is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file LICENSE.txt at the root directory of this source
 * distribution for additional information about the GNU GPL.
 *
 * For using ViSP with software that can not be combined with the GNU
 * GPL, please contact Inria about acquiring a ViSP Professional
 * Edition License.
 *
 * See http://visp.inria.fr for more information.
 *
 * This software was developed at:
 * Inria Rennes - Bretagne Atlantique
 * Campus Universitaire de Beaulieu
 * 35042 Rennes Cedex
 * France
 *
 * If you have questions regarding the use of this file, please contact
 * Inria at visp@inria.fr
 *
 * This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 * WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Description:
 * Interface for Kinova Jaco robot.
 *
 * Authors:
 * Fabien Spindler
 *
 *****************************************************************************/

#include <visp3/core/vpConfig.h>

#ifdef VISP_HAVE_JACOSDK

#include <visp3/core/vpIoTools.h>
#include <visp3/robot/vpRobotException.h>

 /*!
   \file vpRobotKinova.cpp
   \brief Interface for Kinova Jaco2 robot.
 */

#include <visp3/core/vpHomogeneousMatrix.h>
#include <visp3/robot/vpRobotKinova.h>

 /*!
   Default constructor.
  */
vpRobotKinova::vpRobotKinova()
  : m_eMc(), m_plugin_location("./"), m_verbose(false), m_plugin_loaded(false), m_devices_count(0),
  m_devices_list(NULL), m_active_device(-1), m_command_layer(CMD_LAYER_UNSET), m_command_layer_handle()
{
  init();
}

/*!
  Destructor.
 */
vpRobotKinova::~vpRobotKinova()
{
  closePlugin();

  if (m_devices_list) {
    delete[] m_devices_list;
  }
}

/*!
Basic initialization.
*/
void vpRobotKinova::init()
{
  // If you want to control the robot in Cartesian in a tool frame, set the corresponding transformation in m_eMc
  // that is set to identity by default in the constructor.

  maxRotationVelocity = maxRotationVelocityDefault;
  maxTranslationVelocity = maxTranslationVelocityDefault;

  // Set here the robot degrees of freedom number
  nDof = 6; // Jaco2 has 6 dof

  m_devices_list = new KinovaDevice[MAX_KINOVA_DEVICE];
}

/*

  At least one of these function has to be implemented to control the robot with a
  Cartesian velocity:
  - get_eJe()
  - get_fJe()

*/

/*!
  Get the robot Jacobian expressed in the end-effector frame. This function
  is not implemented. In fact, we don't need it since we can control the robot
  in cartesian in end-effector frame.

  \param[out] eJe : End-effector frame Jacobian.
*/
void vpRobotKinova::get_eJe(vpMatrix &eJe)
{
  if (!m_plugin_loaded) {
    throw(vpException(vpException::fatalError, "Jaco SDK plugin not loaded"));
  }
  if (!m_devices_count) {
    throw(vpException(vpException::fatalError, "No Kinova robot found"));
  }

  (void)eJe;
  std::cout << "Not implemented ! " << std::endl;
}

/*!
  Get the robot Jacobian expressed in the robot reference frame. This function
  is not implemented. In fact, we don't need it since we can control the robot
  in cartesian in end-effector frame.

  \param[out] fJe : Base (or reference) frame Jacobian.
*/
void vpRobotKinova::get_fJe(vpMatrix &fJe)
{
  if (!m_plugin_loaded) {
    throw(vpException(vpException::fatalError, "Jaco SDK plugin not loaded"));
  }
  if (!m_devices_count) {
    throw(vpException(vpException::fatalError, "No Kinova robot found"));
  }

  (void)fJe;
  std::cout << "Not implemented ! " << std::endl;
}

/*

  At least one of these function has to be implemented to control the robot:
  - setCartVelocity()
  - setJointVelocity()

*/

/*!
  Send to the controller a 6-dim velocity twist vector expressed in a Cartesian frame.

  \param[in] frame : Cartesian control frame (either tool frame or end-effector) in which the velocity \e v is expressed.
  Units are m/s for translation and rad/s for rotation velocities.

  \param[in] v : 6-dim vector that contains the 6 components of the velocity twist to send to the robot.
  Units are m/s and rad/s.
*/
void vpRobotKinova::setCartVelocity(const vpRobot::vpControlFrameType frame, const vpColVector &v)
{
  if (v.size() != 6) {
    throw(vpException(vpException::fatalError, "Cannot send a velocity twist vector in tool frame that is not 6-dim (%d)", v.size()));
  }

  vpColVector v_e; // This is the velocity that the robot is able to apply in the end-effector frame
  switch (frame) {
  case vpRobot::TOOL_FRAME: {
    // We have to transform the requested velocity in the end-effector frame.
    // Knowing that the constant transformation between the tool frame and the end-effector frame obtained
    // by extrinsic calibration is set in m_eMc we can compute the velocity twist matrix eVc that transform
    // a velocity twist from tool (or camera) frame into end-effector frame
    vpVelocityTwistMatrix eVc(m_eMc);
    v_e = eVc * v;
    break;
  }

  case vpRobot::END_EFFECTOR_FRAME: {
    v_e = v;
    break;
  }
  case vpRobot::REFERENCE_FRAME:
  case vpRobot::JOINT_STATE:
  case vpRobot::MIXT_FRAME:
    // Out of the scope
    break;
  }

  TrajectoryPoint pointToSend;
  pointToSend.InitStruct();
  // We specify that this point will be used an angular (joint by joint) velocity vector
  pointToSend.Position.Type = CARTESIAN_VELOCITY;
  pointToSend.Position.HandMode = HAND_NOMOVEMENT;

  pointToSend.Position.CartesianPosition.X = static_cast<float>(v_e[0]);
  pointToSend.Position.CartesianPosition.Y = static_cast<float>(v_e[1]);
  pointToSend.Position.CartesianPosition.Z = static_cast<float>(v_e[2]);
  pointToSend.Position.CartesianPosition.ThetaX = static_cast<float>(v_e[3]);
  pointToSend.Position.CartesianPosition.ThetaY = static_cast<float>(v_e[4]);
  pointToSend.Position.CartesianPosition.ThetaZ = static_cast<float>(v_e[5]);

  KinovaSetCartesianControl(); // Not sure that this function is useful here

  KinovaSendBasicTrajectory(pointToSend);
}

/*!
  Send a joint velocity to the controller.
  \param[in] qdot : Joint velocities vector. Units are rad/s for a robot arm.
 */
void vpRobotKinova::setJointVelocity(const vpColVector &qdot)
{
  TrajectoryPoint pointToSend;
  pointToSend.InitStruct();
  // We specify that this point will be used an angular (joint by joint) velocity vector
  pointToSend.Position.Type = ANGULAR_VELOCITY;
  pointToSend.Position.HandMode = HAND_NOMOVEMENT;
  pointToSend.Position.Actuators.Actuator1 = static_cast<float>(vpMath::deg(qdot[0]));
  pointToSend.Position.Actuators.Actuator2 = static_cast<float>(vpMath::deg(qdot[1]));
  pointToSend.Position.Actuators.Actuator3 = static_cast<float>(vpMath::deg(qdot[2]));
  pointToSend.Position.Actuators.Actuator4 = static_cast<float>(vpMath::deg(qdot[3]));
  pointToSend.Position.Actuators.Actuator5 = static_cast<float>(vpMath::deg(qdot[4]));
  pointToSend.Position.Actuators.Actuator6 = static_cast<float>(vpMath::deg(qdot[5]));

  KinovaSetAngularControl(); // Not sure that this function is useful here

  KinovaSendBasicTrajectory(pointToSend);
}

/*!
  Send to the controller a velocity in a given frame.

  \param[in] frame : Control frame in which the velocity \e vel is expressed.
  Velocities could be joint velocities, or cartesian velocities. Units are m/s for translation and
  rad/s for rotation velocities.

  \param[in] vel : Vector that contains the velocity to apply to the robot.
 */
void vpRobotKinova::setVelocity(const vpRobot::vpControlFrameType frame, const vpColVector &vel)
{
  if (!m_plugin_loaded) {
    throw(vpException(vpException::fatalError, "Jaco SDK plugin not loaded"));
  }
  if (!m_devices_count) {
    throw(vpException(vpException::fatalError, "No Kinova robot found"));
  }
  if (vpRobot::STATE_VELOCITY_CONTROL != getRobotState()) {
    throw vpRobotException(vpRobotException::wrongStateError,
      "Cannot send a velocity to the robot. "
      "Call setRobotState(vpRobot::STATE_VELOCITY_CONTROL) once before "
      "entering your control loop.");
  }

  vpColVector vel_sat(6);

  // Velocity saturation
  switch (frame) {
  // Saturation in cartesian space
  case vpRobot::TOOL_FRAME:
  case vpRobot::REFERENCE_FRAME:
  case vpRobot::END_EFFECTOR_FRAME:
  case vpRobot::MIXT_FRAME: {
    if (vel.size() != 6) {
      throw(vpException(vpException::dimensionError, "Cannot apply a Cartesian velocity that is not a 6-dim vector (%d)", vel.size()));
    }
    vpColVector vel_max(6);

    for (unsigned int i = 0; i < 3; i++)
      vel_max[i] = getMaxTranslationVelocity();
    for (unsigned int i = 3; i < 6; i++)
      vel_max[i] = getMaxRotationVelocity();

    vel_sat = vpRobot::saturateVelocities(vel, vel_max, true);

    setCartVelocity(frame, vel_sat);
    break;
  }
  
  // Saturation in joint space
  case vpRobot::JOINT_STATE: {
    if (vel.size() != static_cast<size_t>(nDof)) {
      throw(vpException(vpException::dimensionError, "Cannot apply a joint velocity that is not a %-dim vector (%d)", nDof, vel.size()));
    }
    vpColVector vel_max(vel.size());

    // Since the robot has only rotation axis all the joint max velocities are set to getMaxRotationVelocity()
    vel_max = getMaxRotationVelocity();

    vel_sat = vpRobot::saturateVelocities(vel, vel_max, true);

    setJointVelocity(vel_sat);
  }
  }
}

/*

  THESE FUNCTIONS ARE NOT MENDATORY BUT ARE USUALLY USEFUL

*/

/*!
  Get robot joint positions.

  \warning We consider here that the robot has only 6 dof, but from the Jaco SDK it could be 7. Should be improved.

  \param[in] q : Joint position in rad.
 */
void vpRobotKinova::getJointPosition(vpColVector &q)
{
  AngularPosition currentCommand;

  // We get the actual angular command of the robot. Values are in deg
  KinovaGetAngularCommand(currentCommand);

  q.resize(6);
  q[0] = vpMath::rad(currentCommand.Actuators.Actuator1);
  q[1] = vpMath::rad(currentCommand.Actuators.Actuator2);
  q[2] = vpMath::rad(currentCommand.Actuators.Actuator3);
  q[3] = vpMath::rad(currentCommand.Actuators.Actuator4);
  q[4] = vpMath::rad(currentCommand.Actuators.Actuator5);
  q[5] = vpMath::rad(currentCommand.Actuators.Actuator6);
}

/*!
  Get robot position.

  \param[in] frame : Considered cartesian frame or joint state.
  \param[out] q : Position of the arm. Joint angles are expressed in rad,
  while cartesian position are expressed in meter for translations and radian
  for rotations.
 */
void vpRobotKinova::getPosition(const vpRobot::vpControlFrameType frame, vpColVector &q)
{
  if (!m_plugin_loaded) {
    throw(vpException(vpException::fatalError, "Jaco SDK plugin not loaded"));
  }
  if (!m_devices_count) {
    throw(vpException(vpException::fatalError, "No Kinova robot found"));
  }

  if (frame == JOINT_STATE) {
    getJointPosition(q);
  }
  else if (frame == END_EFFECTOR_FRAME) {
    CartesianPosition currentCommand;
    // We get the actual cartesian position of the robot
    KinovaGetCartesianCommand(currentCommand);
    q.resize(6);
    q[0] = currentCommand.Coordinates.X;
    q[1] = currentCommand.Coordinates.Y;
    q[2] = currentCommand.Coordinates.Z;
    q[3] = currentCommand.Coordinates.ThetaX;
    q[4] = currentCommand.Coordinates.ThetaY;
    q[5] = currentCommand.Coordinates.ThetaZ;
  }
  else {
    std::cout << "Not implemented ! " << std::endl;
  }
}

/*!
  Set a position to reach.

  \param[in] frame : Considered cartesian frame or joint state.
  \param[in] q : Position to reach.
 */
void vpRobotKinova::setPosition(const vpRobot::vpControlFrameType frame, const vpColVector &q)
{
  if (!m_plugin_loaded) {
    throw(vpException(vpException::fatalError, "Jaco SDK plugin not loaded"));
  }
  if (!m_devices_count) {
    throw(vpException(vpException::fatalError, "No Kinova robot found"));
  }
  if (frame == vpRobot::JOINT_STATE) {
    if (q.size() != nDof) {
      throw(vpException(vpException::fatalError, "Cannot move robot to a joint position of dim %d that is not a %d-dim vector", q.size(), nDof));
    }
    TrajectoryPoint pointToSend;
    pointToSend.InitStruct();
    // We specify that this point will be an angular(joint by joint) position.
    pointToSend.Position.Type = ANGULAR_POSITION;
    pointToSend.Position.HandMode = HAND_NOMOVEMENT;
    pointToSend.Position.Actuators.Actuator1 = static_cast<float>(vpMath::deg(q[0]));
    pointToSend.Position.Actuators.Actuator2 = static_cast<float>(vpMath::deg(q[1]));
    pointToSend.Position.Actuators.Actuator3 = static_cast<float>(vpMath::deg(q[2]));
    pointToSend.Position.Actuators.Actuator4 = static_cast<float>(vpMath::deg(q[3]));
    pointToSend.Position.Actuators.Actuator5 = static_cast<float>(vpMath::deg(q[4]));
    pointToSend.Position.Actuators.Actuator6 = static_cast<float>(vpMath::deg(q[5]));

    KinovaSetAngularControl(); // Not sure that this function is useful here

    if (m_verbose) {
      std::cout << "Move robot to joint position [rad rad rad rad rad rad]: " << q.t() << std::endl;
    }
    KinovaSendBasicTrajectory(pointToSend);
  }
  else if (frame == vpRobot::END_EFFECTOR_FRAME) {
    if (q.size() != 6) {
      throw(vpException(vpException::fatalError, "Cannot move robot to cartesian position of dim %d that is not a 6-dim vector", q.size()));
    }
    TrajectoryPoint pointToSend;
    pointToSend.InitStruct();
    // We specify that this point will be an angular(joint by joint) position.
    pointToSend.Position.Type = CARTESIAN_POSITION;
    pointToSend.Position.HandMode = HAND_NOMOVEMENT;
    pointToSend.Position.CartesianPosition.X = static_cast<float>(q[0]);
    pointToSend.Position.CartesianPosition.Y = static_cast<float>(q[1]);
    pointToSend.Position.CartesianPosition.Z = static_cast<float>(q[2]);
    pointToSend.Position.CartesianPosition.ThetaX = static_cast<float>(q[3]);
    pointToSend.Position.CartesianPosition.ThetaY = static_cast<float>(q[4]);
    pointToSend.Position.CartesianPosition.ThetaZ = static_cast<float>(q[5]);

    KinovaSetCartesianControl(); // Not sure that this function is useful here

    if (m_verbose) {
      std::cout << "Move robot to cartesian position [m m m rad rad rad]: " << q.t() << std::endl;
    }
    KinovaSendBasicTrajectory(pointToSend);

  }
  else {
    throw(vpException(vpException::fatalError, "Cannot move robot to a cartesian position. Only joint positioning is implemented"));
  }
}

/*!
  Get a displacement.

  \param[in] frame : Considered cartesian frame or joint state.
  \param[out] q : Displacement in meter and rad.
 */
void vpRobotKinova::getDisplacement(const vpRobot::vpControlFrameType frame, vpColVector &q)
{
  if (!m_plugin_loaded) {
    throw(vpException(vpException::fatalError, "Jaco SDK plugin not loaded"));
  }
  if (!m_devices_count) {
    throw(vpException(vpException::fatalError, "No Kinova robot found"));
  }

  (void)frame;
  (void)q;
  std::cout << "Not implemented ! " << std::endl;
}

/*!
  Load functions from Jaco SDK plugin.
  - When command layer is set to CMD_LAYER_USB we load `Kinova.API.USBCommandLayerUbuntu.so` or `CommandLayerWindows.dll`
  respectively on unix-like or Windows platform.
  - When command layer is set to CMD_LAYER_ETHERNET we load `Kinova.API.EthCommandLayerUbuntu.so` or `CommandLayerEthernet.dll`
  respectively on unix-like or Windows platform.

  There is setPluginLocation() that allows to modify the default location of the plugin set to "./".

  \sa setPluginLocation(), setCommandLayer()
*/
void vpRobotKinova::loadPlugin()
{
  if (m_command_layer == CMD_LAYER_UNSET) {
    throw(vpException(vpException::fatalError, "Kinova robot command layer unset"));
  }
  if (m_plugin_loaded) {
    closePlugin();
  }
#ifdef __linux__ 
  // We load the API
  std::string plugin_name = (m_command_layer == CMD_LAYER_USB) ? std::string("Kinova.API.USBCommandLayerUbuntu.so") : std::string("Kinova.API.EthCommandLayerUbuntu.so");
  std::string plugin = vpIoTools::createFilePath(m_plugin_location, "Kinova.API.USBCommandLayerUbuntu.so");
  if (m_verbose) {
    std::cout << "Load plugin: \"" << plugin << "\"" << std::endl;
  }
  m_command_layer_handle = dlopen(plugin.c_str(), RTLD_NOW | RTLD_GLOBAL);

  std::string prefix = (m_command_layer == CMD_LAYER_USB) ? std::string("") : std::string("Ethernet_");
  // We load the functions from the library
  KinovaCloseAPI = (int(*)()) dlsym(m_command_layer_handle, (prefix + std::string("CloseAPI");
  KinovaGetAngularCommand = (int(*)(AngularPosition &)) dlsym(m_command_layer_handle, (prefix + std::string("GetAngularCommand")).c_str());
  KinovaGetCartesianCommand = (int(*)(CartesianPosition &)) dlsym(m_command_layer_handle, (prefix + std::string("GetCartesianCommand")).c_str());
  KinovaGetDevices = (int(*)(KinovaDevice devices[MAX_KINOVA_DEVICE], int &result)) dlsym(m_command_layer_handle, (prefix + std::string("GetDevices")).c_str());
  KinovaInitAPI = (int(*)()) dlsym(m_command_layer_handle, (prefix + std::string("InitAPI")).c_str());
  KinovaInitFingers = (int(*)()) dlsym(m_command_layer_handle, (prefix + std::string("InitFingers")).c_str());
  KinovaMoveHome = (int(*)()) dlsym(m_command_layer_handle, (prefix + std::string("MoveHome")).c_str());
  KinovaSendBasicTrajectory = (int(*)(TrajectoryPoint)) dlsym(m_command_layer_handle, (prefix + std::string("SendBasicTrajectory")).c_str());
  KinovaSetActiveDevice = (int(*)(KinovaDevice devices)) dlsym(m_command_layer_handle, (prefix + std::string("SetActiveDevice")).c_str());
  KinovaSetAngularControl = (int(*)()) dlsym(m_command_layer_handle, (prefix + std::string("SetAngularControl")).c_str());
  KinovaSetCartesianControl = (int(*)()) dlsym(m_command_layer_handle, (prefix + std::string("SetCartesianControl")).c_str());
#elif _WIN32
  // We load the API.
  std::string plugin_name = (m_command_layer == CMD_LAYER_USB) ? std::string("CommandLayerWindows.dll") : std::string("CommandLayerEthernet.dll");
  std::string plugin = vpIoTools::createFilePath(m_plugin_location, plugin_name);
  if (m_verbose) {
    std::cout << "Load plugin: \"" << plugin << "\"" << std::endl;
  }
  m_command_layer_handle = LoadLibrary(TEXT(plugin.c_str()));

  // We load the functions from the library
  KinovaCloseAPI = (int(*)()) GetProcAddress(m_command_layer_handle, "CloseAPI");
  KinovaGetAngularCommand = (int(*)(AngularPosition &)) GetProcAddress(m_command_layer_handle, "GetAngularCommand");
  KinovaGetCartesianCommand = (int(*)(CartesianPosition &)) GetProcAddress(m_command_layer_handle, "GetCartesianCommand");
  KinovaGetDevices = (int(*)(KinovaDevice[MAX_KINOVA_DEVICE], int&)) GetProcAddress(m_command_layer_handle, "GetDevices");
  KinovaInitAPI = (int(*)()) GetProcAddress(m_command_layer_handle, "InitAPI");
  KinovaInitFingers = (int(*)()) GetProcAddress(m_command_layer_handle, "InitFingers");
  KinovaMoveHome = (int(*)()) GetProcAddress(m_command_layer_handle, "MoveHome");
  KinovaSendBasicTrajectory = (int(*)(TrajectoryPoint)) GetProcAddress(m_command_layer_handle, "SendBasicTrajectory");
  KinovaSetActiveDevice = (int(*)(KinovaDevice)) GetProcAddress(m_command_layer_handle, "SetActiveDevice");
  KinovaSetAngularControl = (int(*)()) GetProcAddress(m_command_layer_handle, "SetAngularControl");
  KinovaSetCartesianControl = (int(*)()) GetProcAddress(m_command_layer_handle, "SetCartesianControl");
#endif

  // Verify that all functions has been loaded correctly
  if ((KinovaCloseAPI == NULL) ||
    (KinovaGetAngularCommand == NULL) || (KinovaGetCartesianCommand == NULL) || (KinovaGetDevices == NULL) ||
    (KinovaInitAPI == NULL) || (KinovaInitFingers == NULL) ||
    (KinovaMoveHome == NULL) || (KinovaSendBasicTrajectory == NULL) ||
    (KinovaSetActiveDevice == NULL) || (KinovaSetAngularControl == NULL) || (KinovaSetCartesianControl == NULL)) {
    throw(vpException(vpException::fatalError, "Cannot load plugin from \"%s\" folder", m_plugin_location.c_str()));
  }
  if (m_verbose) {
    std::cout << "Plugin successfully loaded" << std::endl;
  }

  m_plugin_loaded = true;
}

/*!
Close plugin.
*/
void vpRobotKinova::closePlugin()
{
  if (m_plugin_loaded) {
    if (m_verbose) {
      std::cout << "Close plugin" << std::endl;
    }
#ifdef __linux__ 
    dlclose(m_command_layer_handle);
#elif _WIN32
    FreeLibrary(m_command_layer_handle);
#endif
    m_plugin_loaded = false;
  }
}

/*!
  Move the robot to home position.
*/
void vpRobotKinova::homing()
{
  if (!m_plugin_loaded) {
    throw(vpException(vpException::fatalError, "Jaco SDK plugin not loaded"));
  }
  if (!m_devices_count) {
    throw(vpException(vpException::fatalError, "No Kinova robot found"));
  }

  if (m_verbose) {
    std::cout << "Move the robot to home position" << std::endl;
  }
  KinovaMoveHome();
}

/*!
  Connect to Kinova devices.
  \return Number of devices that are connected.
*/
unsigned int vpRobotKinova::connect()
{
  loadPlugin();
  if (!m_plugin_loaded) {
    throw(vpException(vpException::fatalError, "Jaco SDK plugin not loaded"));
  }

  int result = (*KinovaInitAPI)();

  if (m_verbose) {
    std::cout << "Initialization's result: " << result << std::endl;
  }

  m_devices_count = KinovaGetDevices(m_devices_list, result);

  if (m_verbose) {
    std::cout << "Found " << m_devices_count << " devices" << std::endl;
  }

  // By default set the first device as active
  setActiveDevice(0);

  // Initialize fingers
  KinovaInitFingers();

  return m_devices_count;
}

/*!
  Set active device.
  \param[in] device : Device id corresponding to the active device. The first device has id 0.
  The last device is is given by getNumDevices() - 1.
  By default, the active device is the first one.

  To know how many devices are connected, use getNumDevices().

  \sa getActiveDevice()
*/
void vpRobotKinova::setActiveDevice(int device)
{
  if (!m_plugin_loaded) {
    throw(vpException(vpException::fatalError, "Jaco SDK plugin not loaded"));
  }
  if (!m_devices_count) {
    throw(vpException(vpException::fatalError, "No Kinova robot found"));
  }
  if (device < 0 || device >= m_devices_count) {
    throw(vpException(vpException::badValue, "Cannot set Kinova active device %d. Value should be in range [0, %d]", device, m_devices_count - 1));
  }
  if (device != m_active_device) {
    m_active_device = device;
    KinovaSetActiveDevice(m_devices_list[m_active_device]);
  }
}

#elif !defined(VISP_BUILD_SHARED_LIBS)
 // Work arround to avoid warning: libvisp_robot.a(vpRobotKinova.cpp.o) has
 // no symbols
void dummy_vpRobotKinova() {};
#endif
