/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
#include "kdl_cp/chainfksolverpos_recursive.hpp"
#include "kdl_cp/chainiksolverpos_nr_jl.hpp"
#include "kdl_cp/chainiksolvervel_pinv.hpp"
#endif

#include <Base/FileInfo.h>
#include <Base/Reader.h>
#include <Base/Stream.h>
#include <Base/Tools.h>
#include <Base/Writer.h>

#include "Robot6Axis.h"
#include "RobotAlgos.h"


using namespace Robot;
using namespace Base;
using namespace KDL;

// clang-format off
// some default roboter
AxisDefinition KukaIR500[6] = {
//   a    ,alpha ,d    ,theta ,rotDir ,maxAngle ,minAngle ,AxisVelocity
    {500  ,-90   ,1045 ,0     , -1    ,+185     ,-185     ,156         }, // Axis 1
    {1300 ,0     ,0    ,0     , 1     ,+35      ,-155     ,156         }, // Axis 2
    {55   ,+90   ,0    ,-90   , 1     ,+154     ,-130     ,156         }, // Axis 3
    {0    ,-90   ,-1025,0     , 1     ,+350     ,-350     ,330         }, // Axis 4
    {0    ,+90   ,0    ,0     , 1     ,+130     ,-130     ,330         }, // Axis 5
    {0    ,+180  ,-300 ,0     , 1     ,+350     ,-350     ,615         }  // Axis 6
};
// clang-format on


TYPESYSTEM_SOURCE(Robot::Robot6Axis, Base::Persistence)

Robot6Axis::Robot6Axis()
{
    // create joint array for the min and max angle values of each joint
    Min = JntArray(6);
    Max = JntArray(6);

    // Create joint array
    Actual = JntArray(6);

    // set to default kuka 500
    setKinematic(KukaIR500);
}

void Robot6Axis::setKinematic(const AxisDefinition KinDef[6])
{
    Chain temp;


    for (int i = 0; i < 6; i++) {
        temp.addSegment(Segment(Joint(Joint::RotZ),
                                Frame::DH(KinDef[i].a,
                                          Base::toRadians<double>(KinDef[i].alpha),
                                          KinDef[i].d,
                                          Base::toRadians<double>(KinDef[i].theta))));
        RotDir[i] = KinDef[i].rotDir;
        Max(i) = Base::toRadians<double>(KinDef[i].maxAngle);
        Min(i) = Base::toRadians<double>(KinDef[i].minAngle);
        Velocity[i] = KinDef[i].velocity;
    }

    // for now and testing
    Kinematic = temp;

    // get the actual TCP out of the axis
    calcTcp();
}

double Robot6Axis::getMaxAngle(int Axis)
{
    return Base::toDegrees<double>(Max(Axis));
}

double Robot6Axis::getMinAngle(int Axis)
{
    return Base::toDegrees<double>(Min(Axis));
}

void split(std::string const& string, const char delimiter, std::vector<std::string>& destination)
{
    std::string::size_type last_position(0);
    std::string::size_type position(0);

    for (std::string::const_iterator it(string.begin()); it != string.end(); ++it, ++position) {
        if (*it == delimiter) {
            destination.push_back(string.substr(last_position, position - last_position));
            last_position = position + 1;
        }
    }
    destination.push_back(string.substr(last_position, position - last_position));
}

void Robot6Axis::readKinematic(const char* FileName)
{
    char buf[120];
    Base::FileInfo fi(FileName);
    Base::ifstream in(fi);
    if (!in) {
        return;
    }

    std::vector<std::string> destination;
    AxisDefinition temp[6];

    // over read the header
    in.getline(buf, 119, '\n');
    // read 6 Axis
    for (auto& i : temp) {
        in.getline(buf, 79, '\n');
        destination.clear();
        split(std::string(buf), ',', destination);
        if (destination.size() < 8) {
            continue;
        }
        // transfer the values in kinematic structure
        i.a = atof(destination[0].c_str());
        i.alpha = atof(destination[1].c_str());
        i.d = atof(destination[2].c_str());
        i.theta = atof(destination[3].c_str());
        i.rotDir = atof(destination[4].c_str());
        i.maxAngle = atof(destination[5].c_str());
        i.minAngle = atof(destination[6].c_str());
        i.velocity = atof(destination[7].c_str());
    }

    setKinematic(temp);
}

unsigned int Robot6Axis::getMemSize() const
{
    return 0;
}

void Robot6Axis::Save(Writer& writer) const
{
    for (unsigned int i = 0; i < 6; i++) {
        Base::Placement Tip = toPlacement(Kinematic.getSegment(i).getFrameToTip());
        writer.Stream() << writer.ind() << "<Axis "
                        << "Px=\"" << Tip.getPosition().x << "\" "
                        << "Py=\"" << Tip.getPosition().y << "\" "
                        << "Pz=\"" << Tip.getPosition().z << "\" "
                        << "Q0=\"" << Tip.getRotation()[0] << "\" "
                        << "Q1=\"" << Tip.getRotation()[1] << "\" "
                        << "Q2=\"" << Tip.getRotation()[2] << "\" "
                        << "Q3=\"" << Tip.getRotation()[3] << "\" "
                        << "rotDir=\"" << RotDir[i] << "\" "
                        << "maxAngle=\"" << Base::toDegrees<double>(Max(i)) << "\" "
                        << "minAngle=\"" << Base::toDegrees<double>(Min(i)) << "\" "
                        << "AxisVelocity=\"" << Velocity[i] << "\" "
                        << "Pos=\"" << Actual(i) << "\"/>" << std::endl;
    }
}

void Robot6Axis::Restore(XMLReader& reader)
{
    Chain Temp;
    Base::Placement Tip;

    for (unsigned int i = 0; i < 6; i++) {
        // read my Element
        reader.readElement("Axis");
        // get the value of the placement
        Tip = Base::Placement(Base::Vector3d(reader.getAttribute<double>("Px"),
                                             reader.getAttribute<double>("Py"),
                                             reader.getAttribute<double>("Pz")),
                              Base::Rotation(reader.getAttribute<double>("Q0"),
                                             reader.getAttribute<double>("Q1"),
                                             reader.getAttribute<double>("Q2"),
                                             reader.getAttribute<double>("Q3")));
        Temp.addSegment(Segment(Joint(Joint::RotZ), toFrame(Tip)));


        if (reader.hasAttribute("rotDir")) {
            Velocity[i] = reader.getAttribute<double>("rotDir");
        }
        else {
            Velocity[i] = 1.0;
        }
        // read the axis constraints
        Min(i) = Base::toRadians<double>(reader.getAttribute<double>("maxAngle"));
        Max(i) = Base::toRadians<double>(reader.getAttribute<double>("minAngle"));
        if (reader.hasAttribute("AxisVelocity")) {
            Velocity[i] = reader.getAttribute<double>("AxisVelocity");
        }
        else {
            Velocity[i] = 156.0;
        }
        Actual(i) = reader.getAttribute<double>("Pos");
    }
    Kinematic = Temp;

    calcTcp();
}

bool Robot6Axis::setTo(const Placement& To)
{
    // Creation of the solvers:
    ChainFkSolverPos_recursive fksolver1(Kinematic);  // Forward position solver
    ChainIkSolverVel_pinv iksolver1v(Kinematic);      // Inverse velocity solver
    ChainIkSolverPos_NR_JL iksolver1(Kinematic,
                                     Min,
                                     Max,
                                     fksolver1,
                                     iksolver1v,
                                     100,
                                     1e-6);  // Maximum 100 iterations, stop at accuracy 1e-6

    // Creation of jntarrays:
    JntArray result(Kinematic.getNrOfJoints());

    // Set destination frame
    Frame F_dest =
        Frame(KDL::Rotation::Quaternion(To.getRotation()[0],
                                        To.getRotation()[1],
                                        To.getRotation()[2],
                                        To.getRotation()[3]),
              KDL::Vector(To.getPosition()[0], To.getPosition()[1], To.getPosition()[2]));

    // solve
    if (iksolver1.CartToJnt(Actual, F_dest, result) < 0) {
        return false;
    }
    else {
        Actual = result;
        Tcp = F_dest;
        return true;
    }
}

Base::Placement Robot6Axis::getTcp()
{
    double x, y, z, w;
    Tcp.M.GetQuaternion(x, y, z, w);
    return Base::Placement(Base::Vector3d(Tcp.p[0], Tcp.p[1], Tcp.p[2]),
                           Base::Rotation(x, y, z, w));
}

bool Robot6Axis::calcTcp()
{
    // Create solver based on kinematic chain
    ChainFkSolverPos_recursive fksolver = ChainFkSolverPos_recursive(Kinematic);

    // Create the frame that will contain the results
    KDL::Frame cartpos;

    // Calculate forward position kinematics
    int kinematics_status;
    kinematics_status = fksolver.JntToCart(Actual, cartpos);
    if (kinematics_status >= 0) {
        Tcp = cartpos;
        return true;
    }
    else {
        return false;
    }
}

bool Robot6Axis::setAxis(int Axis, double Value)
{
    Actual(Axis) = RotDir[Axis] * Base::toRadians<double>(Value);
    return calcTcp();
}

double Robot6Axis::getAxis(int Axis)
{
    return RotDir[Axis] * Base::toDegrees<double>(Actual(Axis));
}
