/**
 * @file
 * @author  Chrisitan Urich <christian.urich@gmail.com>
 * @version 1.0
 * @section LICENSE
 *
 * This file is part of DynaMind
 *
 * Copyright (C) 2011-2012  Christian Urich

 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

/**
  * @addtogroup DynaMind-Core
  */
#ifndef DMMODULE_H
#define DMMODULE_H

#include "dmcompilersettings.h"
#include <boost/shared_ptr.hpp>
#include <list>
#include <map>
#include <boost/unordered_map.hpp>
#include <QString>
#include <iostream>
#include <dmresultobserver.h>
#include <dmlog.h>
#include <dmlogger.h>
#include <dmlogsink.h>
#include <dmview.h>
//#include <boost/python.hpp>

class QThreadPool;
class PortObserver;
namespace DM {
class System;
}

namespace DM {



enum  DATATYPES {
    INT,
    LONG,
    DOUBLE,
    STRING,
    FILENAME,
    STRING_LIST,
    STRING_MAP,
    BOOL,
    LASTPRIMITIVETYPE,

    USER_DEFINED_INPUT,

    SYSTEM_OUT,
    SYSTEM_IN,
    SYSTEM
};

enum PORTTYPES {
    OUTSYSTEM,
    OUTTUPLESYSTEM,
    OUTPORTS,

    INSYSTEM,
    INTUPLESYSTEM,
    INPORTS
};

enum CORINE {
    ContUrbanFabric = 2,
    DisContUrbanFabric = 3,
    RoadRailNetwork = 4,
    AgriculturalAreas = 5,
    ForestsSemiNatural = 6,
    WaterBodies = 7
};


struct ModulePrivate;

typedef boost::unordered_map<std::string, std::string> parameter_type;

class Port;
class Group;
class Simulation;
class RasterData;


/**
* @class DM::Module
*
*
* @ingroup DynaMind-Core
*
* @brief all the fun happens in here.
*
* @section Implementation
* @TODO: write stuff here
* @section Development
* To create a new DynaMind module the new module has to be derived from the module class
*
* @subsection C++
*
*
* Example Code
*
* mymodule.h
* @code
*
* #include "dmmodule.h"
*
* using namespace DM;
* class DM_HELPER_DLL_EXPORT MyModule : public Module
* {
*     DM_DECLARE_NODE(MyModule)
*     public:
*         MyModule();
*         void run();
* };
* @endcode
*
* mymodule.cpp
* @code
*
* #include "mymodule.h"
*
* DM_DECLARE_NODE_NAME(MyModule, MyModuleGroup)
* MyModule::MyModule()
* {
* }
* void MyModule::run()
* {
* }
* @endcode
*
* To add the Module to DynaMind
* mymodules.cpp
* @code
* #include "dmnodefactory.h"
* #include "dmmoduleregistry.h"
* #include "mymodule.h"
* #include "mymodule2.h"
* extern "C" void DM_HELPER_DLL_EXPORT  registerModules(ModuleRegistry *registry) {
*      registry->addNodeFactory(new NodeFactory<MyModule>());
*      registry->addNodeFactory(new NodeFactory<MyModule2>());
* }
* @endcode
* @author Christian Urich
*
*/
class DM_HELPER_DLL_EXPORT  Module {


public:
    Module();
    virtual ~Module();

    virtual void Destructor();
    /** @brief Is used to update the initalisation of the module dynamically during the runtime
     *
     * TODO: Include detailed description of the init function
     */
    virtual void init();
    /** @brief The pure virtual run method is where the fun is happening */
    virtual void run() = 0;

    /** @brief Returns the current system state
      *
      * If the succeeding module changes the data a new state of the system has to be created! */
    virtual  DM::System* getSystemState(const std::string &name);


    /** @brief Returns a pointer to the system that is linked to the inport of the module.
      * If no system can be found the method returns 0
      *
      * I ports have to be linked to a standard link and in additionally to a back link. If the internal counter of a the module is 0 (the module is)
      * called the first time. The method tries to get the data from the standard link. If the counter > 0 the back link is used.
      */
    virtual  DM::System* getSystemData(const std::string &name) ;

    /** @brief Creates a new system and adds the corresponding views */
    virtual  DM::System* getSystem_Write(string name, std::vector<View> view);


    virtual int getID() const;
    void setID(const int id);

    /** @brief adds a Parameter to the module.
      *
      * Parameters can be:
      * - DM::DOUBLE
      * - DM::INT
      * - DM::BOOL
      * - DM::STRING
      * - DM::FILENAME
      * - DM::LONG
      * - DM::STRING_LIST
      * - DM::STRING_MAP
      */
    void addParameter(std::string name, int type, void * ref, std::string description = "");


    /** @brief Used to define the data that are used in the module.
      *
      * The data are defined as a vetor of views. For every view in the vector a port according to the AccessType is created.
      * - AccessType Read:   Inport
      * - AccessType Write:  Outport
      * - AccessType Modify: In and Outport
      *
      * The name of the port is defined by the name of the View. If a port already exists no new port is added.
      */
    void addData(std::string name, std::vector<DM::View> view);

    /** @brief Returns a pointer to the system where the view is stored
      *
      * The pointer to the system is updated by the updateParameter method. The updateParameter
      * method is always called before the run method.
      * Follwing system states are returnet. T is the system state before at the outport of the
      * module that provides the data
      * - AccessType Read:   T
      * - AccessType Write:  T+1
      * - AccessType Modify: T+1
      */
    DM::System * getData(std::string dataname);

    /** @brief Returns all views used in the module */
    std::map<std::string, std::vector<DM::View> >  getViews();

    /** @brief Returns all inports */
    std::vector<Port*> getInPorts();

    /** @brief Returns all outports */
    std::vector<Port*> getOutPorts();

    /** @bief Retruns a list of parameters used in the module*/
    std::vector<std::string> getParameterListAsVector()  {return this->parameterList;}

    /** @bief Returns a map of parameters <name, type>*/
    boost::unordered_map<std::string, int> getParameterList()  {return this->parameter;}

    /** @bief return native parameter*/
    template<class T>
    T getParameter(std::string name)   {
        T  * val = (T  *)parameter_vals[name];
        return *val;
    }

    /** @bief set native parameter*/
    template<class T>
    void setParameterNative(std::string name, T val)  {
        T * ref =(T * ) parameter_vals[name];

        *ref = val;
    }


    /** @brief Returns the parameter as string value
      *
      * As seperator for STRING_LIST *|* is used and for maps also *||*
      *
      * 1*|*2*|*3*|4*||*
      *
      * 5*|*6*|*7*|*8*||*
      *
    */
    virtual std::string getParameterAsString(std::string Name);

    /** @brief Set if module is programmed in python*/
    void setPythonModule(bool b) {this->PythonModule = b;}

    /** @brief returns true if module is written in Python*/
    bool isPythonModule(){return PythonModule;}

    /** @brief update parameter and data used in the module.
      *
      * Based on the data used in the module the pointer to the data are updated.
      * If the module reads data from, the method tries to get the system from linked module.
      * Therefore the getSystemData method is called. If the module changes the data a new state of the system
      * is created.
      *
      */
    virtual void updateParameter();

    /** @brief called after the module is executed */
    virtual void postRun();

    /** @brief set parameter as value as string*/
    virtual void setParameterValue(std::string name, std::string value);

    /** @brief remove port
      *
      * removes a port and its links
      * method calls the changedPorts method from the PortObserver
      */
    virtual void removePort(std::string LinkedDataName, int PortType);

    /** @brief add port observer */
    void addPortObserver(PortObserver * portobserver);

    /** @brief return pointer to InPort */
    virtual Port * getInPort(std::string Name);

    /** @brief return pointer to OutPort */
    virtual Port * getOutPort(std::string Name);

    /** @brief return class name */
    virtual  const char * getClassName()  {return "OVERWRITE getClassName";}

    /** @brief return class filename */
    virtual  const char * getFileName()  {return "OVERWRITE getFileName";}

    /** @brief returns if derived from group */
    virtual bool isGroup()const {return false;}

    /** @brief adds the module to a group */
    void setGroup(Group * group);

    /** @brief returns pointer to the group */
    Group * getGroup();

    /** @brief returns uuid */
    std::string getUuid() const {return this->uuid;}

    /** @brief set pointer to simulation */
    void setSimulation(Simulation * simulation);

    /** @brief set name of module */
    void setName(std::string name) {
        this->name = name;
    }

     /** @brief return name of module */
    std::string getName() const {return this->name;}

    /** @brief add ResultObserver */
    void addResultObserver(ResultObserver * ro) {this->resultobserver.push_back(ro);}

    /** @brief resets the paramter in the module
     *
     * Sets the InternalCounter to 0
     */
    void resetParameter();

    /** @brief returns internal counter */
    int getInternalCounter(){return this->internalCounter;}

    /** @brief copies parameter from module A to B is they are from the same type**/
    void copyParameterFromOtherModule(Module * m);

    /** @brief returns a list of PortObserver pointers to the set*/
    std::vector<PortObserver *> getPortObserver(){return this->portobserver;}

    /** @brief returns a list of ResultObserver pointers */
    std::vector<ResultObserver * > getResultObserver(){return this->resultobserver;}

    /** @brief Return simulation **/
    Simulation * getSimulation(){return this->simulation;}

    /** @brief Returns a pointer raster data set assigend to a view **/
    DM::RasterData * getRasterData(std::string dataname, const DM::View & view);

    /** @brief Returns if the module comes with its own GUI.
      *
      * The default value is false. If you develop your own GUI for the module the GUI is
      * overwrite this method in the module implementation, call the GUI within the method
      * an return true.
      */
    virtual bool createInputDialog(){return false;}

private:


    //boost::python::object self;
    bool PythonModule;
    long id;
    void convertValus( void *value, int Type, QString val);
    std::vector<Port*> InPorts;
    std::vector<Port*> OutPorts;

    std::string uuid;
    std::string name;

    Simulation * simulation;
    std::vector<DM::System *> ownedSystems;



protected:
    std::map<std::string, int> int_params;
    boost::unordered_map<std::string, std::string> paramRaw;
    boost::unordered_map<std::string, int> parameter;
    boost::unordered_map<std::string, std::string> parameter_description;
    boost::unordered_map<std::string, DM::System *> data_vals;
    boost::unordered_map<std::string, DM::System *> data_vals_prev;
    boost::unordered_map<std::string, void *> parameter_vals;
    std::map<std::string,std::vector<DM::View> > views;
    std::map<std::string, double> InputDoubleData;
    std::vector<std::string> parameterList;


    /** @brief the internalCounter is increased by on everytime the
     * run method is called.
     */
    int internalCounter;
    Group * group;
    std::vector<PortObserver *> portobserver;
    std::vector<ResultObserver * > resultobserver;

    /** @brief Add a new Port to the Module */
    virtual void addPort(std::string LinkedDataName, int PortType);
};
}


#define DM_DECLARE_NODE(node)  \
    public: \
    static const char *classname; \
    static const char *filename; \
    virtual const char *getClassName() ; \
    virtual const char *getFileName() ; \
    private:

#define  DM_DECLARE_NODE_NAME(nodename, module) \
    const char *nodename::classname = #nodename; \
    const char *nodename::getClassName()  { return nodename::classname; } \
    const char *nodename::filename = #module; \
    const char *nodename::getFileName()  { return nodename::filename; }

#endif // MODULE_H
