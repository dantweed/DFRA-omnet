#
# OMNeT++/OMNEST Makefile for DFRA
#
# This file was generated with the command:
#  opp_makemake -f --deep -O out -I../inet/src -I../inet/src/inet/linklayer/ieee80211/mac -L../inet/out/$$\(CONFIGNAME\)/src -lINET -DINET_IMPORT -KINET_PROJ=../inet
#

# Name of target to be created (-o option)
TARGET = DFRA$(EXE_SUFFIX)

# User interface (uncomment one) (-u option)
USERIF_LIBS = $(ALL_ENV_LIBS) # that is, $(TKENV_LIBS) $(QTENV_LIBS) $(CMDENV_LIBS)
#USERIF_LIBS = $(CMDENV_LIBS)
#USERIF_LIBS = $(TKENV_LIBS)
#USERIF_LIBS = $(QTENV_LIBS)

# C++ include paths (with -I)
INCLUDE_PATH = \
    -I../inet/src \
    -I../inet/src/inet/linklayer/ieee80211/mac \
    -I. \
    -Idfra \
    -Idfra/mac \
    -Idfra/mgmt \
    -Iresults

# Additional object and library files to link with
EXTRA_OBJS =

# Additional libraries (-L, -l options)
LIBS = -L../inet/out/$(CONFIGNAME)/src  -lINET
LIBS += -Wl,-rpath,`abspath ../inet/out/$(CONFIGNAME)/src`

# Output directory
PROJECT_OUTPUT_DIR = out
PROJECTRELATIVE_PATH =
O = $(PROJECT_OUTPUT_DIR)/$(CONFIGNAME)/$(PROJECTRELATIVE_PATH)

# Object files for local .cc, .msg and .sm files
OBJS = \
    $O/dfra/mac/DfraEdcaUpperMac.o \
    $O/dfra/mac/DfraMac.o \
    $O/dfra/mac/DfraUpperMac.o \
    $O/dfra/mgmt/DfraAgentSTA.o \
    $O/dfra/mgmt/DfraMgmtAP.o \
    $O/dfra/mgmt/DfraMgmtSTA.o

# Message files
MSGFILES =

# SM files
SMFILES =

# Other makefile variables (-K)
INET_PROJ=../inet

#------------------------------------------------------------------------------

# Pull in OMNeT++ configuration (Makefile.inc or configuser.vc)

ifneq ("$(OMNETPP_CONFIGFILE)","")
CONFIGFILE = $(OMNETPP_CONFIGFILE)
else
ifneq ("$(OMNETPP_ROOT)","")
CONFIGFILE = $(OMNETPP_ROOT)/Makefile.inc
else
CONFIGFILE = $(shell opp_configfilepath)
endif
endif

ifeq ("$(wildcard $(CONFIGFILE))","")
$(error Config file '$(CONFIGFILE)' does not exist -- add the OMNeT++ bin directory to the path so that opp_configfilepath can be found, or set the OMNETPP_CONFIGFILE variable to point to Makefile.inc)
endif

include $(CONFIGFILE)

# Simulation kernel and user interface libraries
OMNETPP_LIB_SUBDIR = $(OMNETPP_LIB_DIR)/$(TOOLCHAIN_NAME)
OMNETPP_LIBS = -L"$(OMNETPP_LIB_SUBDIR)" -L"$(OMNETPP_LIB_DIR)" -loppmain$D $(USERIF_LIBS) $(KERNEL_LIBS) $(SYS_LIBS)

COPTS = $(CFLAGS) -DINET_IMPORT $(INCLUDE_PATH) -I$(OMNETPP_INCL_DIR)
MSGCOPTS = $(INCLUDE_PATH)
SMCOPTS =

# we want to recompile everything if COPTS changes,
# so we store COPTS into $COPTS_FILE and have object
# files depend on it (except when "make depend" was called)
COPTS_FILE = $O/.last-copts
ifneq ($(MAKECMDGOALS),depend)
ifneq ("$(COPTS)","$(shell cat $(COPTS_FILE) 2>/dev/null || echo '')")
$(shell $(MKPATH) "$O" && echo "$(COPTS)" >$(COPTS_FILE))
endif
endif

#------------------------------------------------------------------------------
# User-supplied makefile fragment(s)
# >>>
# <<<
#------------------------------------------------------------------------------

# Main target
all: $O/$(TARGET)
	$(Q)$(LN) $O/$(TARGET) .

$O/$(TARGET): $(OBJS)  $(wildcard $(EXTRA_OBJS)) Makefile
	@$(MKPATH) $O
	@echo Creating executable: $@
	$(Q)$(CXX) $(LDFLAGS) -o $O/$(TARGET)  $(OBJS) $(EXTRA_OBJS) $(AS_NEEDED_OFF) $(WHOLE_ARCHIVE_ON) $(LIBS) $(WHOLE_ARCHIVE_OFF) $(OMNETPP_LIBS)

.PHONY: all clean cleanall depend msgheaders smheaders

.SUFFIXES: .cc

$O/%.o: %.cc $(COPTS_FILE)
	@$(MKPATH) $(dir $@)
	$(qecho) "$<"
	$(Q)$(CXX) -c $(CXXFLAGS) $(COPTS) -o $@ $<

%_m.cc %_m.h: %.msg
	$(qecho) MSGC: $<
	$(Q)$(MSGC) -s _m.cc $(MSGCOPTS) $?

%_sm.cc %_sm.h: %.sm
	$(qecho) SMC: $<
	$(Q)$(SMC) -c++ -suffix cc $(SMCOPTS) $?

msgheaders: $(MSGFILES:.msg=_m.h)

smheaders: $(SMFILES:.sm=_sm.h)

clean:
	$(qecho) Cleaning...
	$(Q)-rm -rf $O
	$(Q)-rm -f DFRA DFRA.exe libDFRA.so libDFRA.a libDFRA.dll libDFRA.dylib
	$(Q)-rm -f ./*_m.cc ./*_m.h ./*_sm.cc ./*_sm.h
	$(Q)-rm -f dfra/*_m.cc dfra/*_m.h dfra/*_sm.cc dfra/*_sm.h
	$(Q)-rm -f dfra/mac/*_m.cc dfra/mac/*_m.h dfra/mac/*_sm.cc dfra/mac/*_sm.h
	$(Q)-rm -f dfra/mgmt/*_m.cc dfra/mgmt/*_m.h dfra/mgmt/*_sm.cc dfra/mgmt/*_sm.h
	$(Q)-rm -f results/*_m.cc results/*_m.h results/*_sm.cc results/*_sm.h

cleanall: clean
	$(Q)-rm -rf $(PROJECT_OUTPUT_DIR)

depend:
	$(qecho) Creating dependencies...
	$(Q)$(MAKEDEPEND) $(INCLUDE_PATH) -f Makefile -P\$$O/ -- $(MSG_CC_FILES) $(SM_CC_FILES)  ./*.cc dfra/*.cc dfra/mac/*.cc dfra/mgmt/*.cc results/*.cc

# DO NOT DELETE THIS LINE -- make depend depends on it.
$O/dfra/mac/DfraEdcaUpperMac.o: dfra/mac/DfraEdcaUpperMac.cc \
	dfra/mac/DfraEdcaUpperMac.h \
	dfra/mac/DfraMac.h \
	$(INET_PROJ)/src/inet/common/Compat.h \
	$(INET_PROJ)/src/inet/common/DelayedInitializer.h \
	$(INET_PROJ)/src/inet/common/INETDefs.h \
	$(INET_PROJ)/src/inet/common/INETMath.h \
	$(INET_PROJ)/src/inet/common/INETUtils.h \
	$(INET_PROJ)/src/inet/common/InitStages.h \
	$(INET_PROJ)/src/inet/common/LayeredProtocolBase.h \
	$(INET_PROJ)/src/inet/common/ModuleAccess.h \
	$(INET_PROJ)/src/inet/common/NotifierConsts.h \
	$(INET_PROJ)/src/inet/common/Units.h \
	$(INET_PROJ)/src/inet/common/geometry/common/Coord.h \
	$(INET_PROJ)/src/inet/common/geometry/common/EulerAngles.h \
	$(INET_PROJ)/src/inet/common/lifecycle/ILifecycle.h \
	$(INET_PROJ)/src/inet/common/lifecycle/LifecycleOperation.h \
	$(INET_PROJ)/src/inet/common/lifecycle/NodeOperations.h \
	$(INET_PROJ)/src/inet/common/lifecycle/OperationalBase.h \
	$(INET_PROJ)/src/inet/common/mapping/Interpolation.h \
	$(INET_PROJ)/src/inet/common/mapping/MappingBase.h \
	$(INET_PROJ)/src/inet/common/mapping/MappingUtils.h \
	$(INET_PROJ)/src/inet/common/queue/IPassiveQueue.h \
	$(INET_PROJ)/src/inet/features.h \
	$(INET_PROJ)/src/inet/linklayer/base/MACProtocolBase.h \
	$(INET_PROJ)/src/inet/linklayer/common/Ieee802Ctrl_m.h \
	$(INET_PROJ)/src/inet/linklayer/common/MACAddress.h \
	$(INET_PROJ)/src/inet/linklayer/ieee80211/mac/AccessCategory.h \
	$(INET_PROJ)/src/inet/linklayer/ieee80211/mac/DuplicateDetectors.h \
	$(INET_PROJ)/src/inet/linklayer/ieee80211/mac/Fragmentation.h \
	$(INET_PROJ)/src/inet/linklayer/ieee80211/mac/FrameExchange.h \
	$(INET_PROJ)/src/inet/linklayer/ieee80211/mac/FrameExchanges.h \
	$(INET_PROJ)/src/inet/linklayer/ieee80211/mac/IContention.h \
	$(INET_PROJ)/src/inet/linklayer/ieee80211/mac/IDuplicateDetector.h \
	$(INET_PROJ)/src/inet/linklayer/ieee80211/mac/IFragmentation.h \
	$(INET_PROJ)/src/inet/linklayer/ieee80211/mac/IFrameExchange.h \
	$(INET_PROJ)/src/inet/linklayer/ieee80211/mac/IMacParameters.h \
	$(INET_PROJ)/src/inet/linklayer/ieee80211/mac/IMacRadioInterface.h \
	$(INET_PROJ)/src/inet/linklayer/ieee80211/mac/IRateControl.h \
	$(INET_PROJ)/src/inet/linklayer/ieee80211/mac/IRateSelection.h \
	$(INET_PROJ)/src/inet/linklayer/ieee80211/mac/IRx.h \
	$(INET_PROJ)/src/inet/linklayer/ieee80211/mac/IStatistics.h \
	$(INET_PROJ)/src/inet/linklayer/ieee80211/mac/ITx.h \
	$(INET_PROJ)/src/inet/linklayer/ieee80211/mac/ITxCallback.h \
	$(INET_PROJ)/src/inet/linklayer/ieee80211/mac/IUpperMac.h \
	$(INET_PROJ)/src/inet/linklayer/ieee80211/mac/Ieee80211Frame_m.h \
	$(INET_PROJ)/src/inet/linklayer/ieee80211/mac/Ieee80211Mac.h \
	$(INET_PROJ)/src/inet/linklayer/ieee80211/mac/MacParameters.h \
	$(INET_PROJ)/src/inet/linklayer/ieee80211/mac/MacPlugin.h \
	$(INET_PROJ)/src/inet/linklayer/ieee80211/mac/MacUtils.h \
	$(INET_PROJ)/src/inet/mobility/contract/IMobility.h \
	$(INET_PROJ)/src/inet/networklayer/common/InterfaceEntry.h \
	$(INET_PROJ)/src/inet/networklayer/common/InterfaceToken.h \
	$(INET_PROJ)/src/inet/networklayer/common/L3Address.h \
	$(INET_PROJ)/src/inet/networklayer/common/ModuleIdAddress.h \
	$(INET_PROJ)/src/inet/networklayer/common/ModulePathAddress.h \
	$(INET_PROJ)/src/inet/networklayer/contract/IInterfaceTable.h \
	$(INET_PROJ)/src/inet/networklayer/contract/ipv4/IPv4Address.h \
	$(INET_PROJ)/src/inet/networklayer/contract/ipv6/IPv6Address.h \
	$(INET_PROJ)/src/inet/physicallayer/base/packetlevel/PhysicalLayerDefs.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/bitlevel/ISignalAnalogModel.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IAntenna.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IArrival.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IInterference.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IListening.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IListeningDecision.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IModulation.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/INoise.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IPhysicalLayer.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IPhysicalLayerFrame.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IPrintableObject.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IRadio.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IRadioFrame.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IRadioSignal.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IReceiver.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IReception.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IReceptionDecision.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IReceptionResult.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/ISNIR.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/ITransmission.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/ITransmitter.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/RadioControlInfo_m.h \
	$(INET_PROJ)/src/inet/physicallayer/ieee80211/mode/IIeee80211Mode.h \
	$(INET_PROJ)/src/inet/physicallayer/ieee80211/mode/Ieee80211Band.h \
	$(INET_PROJ)/src/inet/physicallayer/ieee80211/mode/Ieee80211Channel.h \
	$(INET_PROJ)/src/inet/physicallayer/ieee80211/mode/Ieee80211ModeSet.h \
	$(INET_PROJ)/src/inet/physicallayer/ieee80211/packetlevel/Ieee80211ControlInfo_m.h
$O/dfra/mac/DfraMac.o: dfra/mac/DfraMac.cc \
	dfra/mac/DfraMac.h \
	$(INET_PROJ)/src/inet/common/Compat.h \
	$(INET_PROJ)/src/inet/common/DelayedInitializer.h \
	$(INET_PROJ)/src/inet/common/INETDefs.h \
	$(INET_PROJ)/src/inet/common/INETMath.h \
	$(INET_PROJ)/src/inet/common/INETUtils.h \
	$(INET_PROJ)/src/inet/common/InitStages.h \
	$(INET_PROJ)/src/inet/common/LayeredProtocolBase.h \
	$(INET_PROJ)/src/inet/common/ModuleAccess.h \
	$(INET_PROJ)/src/inet/common/NotifierConsts.h \
	$(INET_PROJ)/src/inet/common/Units.h \
	$(INET_PROJ)/src/inet/common/geometry/common/Coord.h \
	$(INET_PROJ)/src/inet/common/geometry/common/EulerAngles.h \
	$(INET_PROJ)/src/inet/common/lifecycle/ILifecycle.h \
	$(INET_PROJ)/src/inet/common/lifecycle/LifecycleOperation.h \
	$(INET_PROJ)/src/inet/common/lifecycle/NodeOperations.h \
	$(INET_PROJ)/src/inet/common/lifecycle/OperationalBase.h \
	$(INET_PROJ)/src/inet/common/mapping/Interpolation.h \
	$(INET_PROJ)/src/inet/common/mapping/MappingBase.h \
	$(INET_PROJ)/src/inet/common/mapping/MappingUtils.h \
	$(INET_PROJ)/src/inet/features.h \
	$(INET_PROJ)/src/inet/linklayer/base/MACProtocolBase.h \
	$(INET_PROJ)/src/inet/linklayer/common/Ieee802Ctrl_m.h \
	$(INET_PROJ)/src/inet/linklayer/common/MACAddress.h \
	$(INET_PROJ)/src/inet/linklayer/ieee80211/mac/IContention.h \
	$(INET_PROJ)/src/inet/linklayer/ieee80211/mac/IMacRadioInterface.h \
	$(INET_PROJ)/src/inet/linklayer/ieee80211/mac/IRx.h \
	$(INET_PROJ)/src/inet/linklayer/ieee80211/mac/ITx.h \
	$(INET_PROJ)/src/inet/linklayer/ieee80211/mac/IUpperMac.h \
	$(INET_PROJ)/src/inet/linklayer/ieee80211/mac/Ieee80211Frame_m.h \
	$(INET_PROJ)/src/inet/mobility/contract/IMobility.h \
	$(INET_PROJ)/src/inet/networklayer/common/InterfaceEntry.h \
	$(INET_PROJ)/src/inet/networklayer/common/InterfaceToken.h \
	$(INET_PROJ)/src/inet/networklayer/common/L3Address.h \
	$(INET_PROJ)/src/inet/networklayer/common/ModuleIdAddress.h \
	$(INET_PROJ)/src/inet/networklayer/common/ModulePathAddress.h \
	$(INET_PROJ)/src/inet/networklayer/contract/IInterfaceTable.h \
	$(INET_PROJ)/src/inet/networklayer/contract/ipv4/IPv4Address.h \
	$(INET_PROJ)/src/inet/networklayer/contract/ipv6/IPv6Address.h \
	$(INET_PROJ)/src/inet/physicallayer/base/packetlevel/PhysicalLayerDefs.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/bitlevel/ISignalAnalogModel.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IAntenna.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IArrival.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IInterference.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IListening.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IListeningDecision.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IModulation.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/INoise.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IPhysicalLayer.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IPhysicalLayerFrame.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IPrintableObject.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IRadio.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IRadioFrame.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IRadioSignal.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IReceiver.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IReception.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IReceptionDecision.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IReceptionResult.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/ISNIR.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/ITransmission.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/ITransmitter.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/RadioControlInfo_m.h \
	$(INET_PROJ)/src/inet/physicallayer/ieee80211/mode/IIeee80211Mode.h \
	$(INET_PROJ)/src/inet/physicallayer/ieee80211/mode/Ieee80211Band.h \
	$(INET_PROJ)/src/inet/physicallayer/ieee80211/mode/Ieee80211Channel.h \
	$(INET_PROJ)/src/inet/physicallayer/ieee80211/mode/Ieee80211ModeSet.h \
	$(INET_PROJ)/src/inet/physicallayer/ieee80211/packetlevel/Ieee80211ControlInfo_m.h
$O/dfra/mac/DfraUpperMac.o: dfra/mac/DfraUpperMac.cc \
	dfra/mac/DfraMac.h \
	dfra/mac/DfraUpperMac.h \
	$(INET_PROJ)/src/inet/common/Compat.h \
	$(INET_PROJ)/src/inet/common/DelayedInitializer.h \
	$(INET_PROJ)/src/inet/common/INETDefs.h \
	$(INET_PROJ)/src/inet/common/INETMath.h \
	$(INET_PROJ)/src/inet/common/INETUtils.h \
	$(INET_PROJ)/src/inet/common/InitStages.h \
	$(INET_PROJ)/src/inet/common/LayeredProtocolBase.h \
	$(INET_PROJ)/src/inet/common/ModuleAccess.h \
	$(INET_PROJ)/src/inet/common/NotifierConsts.h \
	$(INET_PROJ)/src/inet/common/Units.h \
	$(INET_PROJ)/src/inet/common/geometry/common/Coord.h \
	$(INET_PROJ)/src/inet/common/geometry/common/EulerAngles.h \
	$(INET_PROJ)/src/inet/common/lifecycle/ILifecycle.h \
	$(INET_PROJ)/src/inet/common/lifecycle/LifecycleOperation.h \
	$(INET_PROJ)/src/inet/common/lifecycle/NodeOperations.h \
	$(INET_PROJ)/src/inet/common/lifecycle/OperationalBase.h \
	$(INET_PROJ)/src/inet/common/mapping/Interpolation.h \
	$(INET_PROJ)/src/inet/common/mapping/MappingBase.h \
	$(INET_PROJ)/src/inet/common/mapping/MappingUtils.h \
	$(INET_PROJ)/src/inet/common/queue/IPassiveQueue.h \
	$(INET_PROJ)/src/inet/features.h \
	$(INET_PROJ)/src/inet/linklayer/base/MACProtocolBase.h \
	$(INET_PROJ)/src/inet/linklayer/common/Ieee802Ctrl_m.h \
	$(INET_PROJ)/src/inet/linklayer/common/MACAddress.h \
	$(INET_PROJ)/src/inet/linklayer/ieee80211/mac/AccessCategory.h \
	$(INET_PROJ)/src/inet/linklayer/ieee80211/mac/DuplicateDetectors.h \
	$(INET_PROJ)/src/inet/linklayer/ieee80211/mac/FrameExchange.h \
	$(INET_PROJ)/src/inet/linklayer/ieee80211/mac/FrameExchanges.h \
	$(INET_PROJ)/src/inet/linklayer/ieee80211/mac/IContention.h \
	$(INET_PROJ)/src/inet/linklayer/ieee80211/mac/IDuplicateDetector.h \
	$(INET_PROJ)/src/inet/linklayer/ieee80211/mac/IFragmentation.h \
	$(INET_PROJ)/src/inet/linklayer/ieee80211/mac/IFrameExchange.h \
	$(INET_PROJ)/src/inet/linklayer/ieee80211/mac/IMacParameters.h \
	$(INET_PROJ)/src/inet/linklayer/ieee80211/mac/IMacRadioInterface.h \
	$(INET_PROJ)/src/inet/linklayer/ieee80211/mac/IRateControl.h \
	$(INET_PROJ)/src/inet/linklayer/ieee80211/mac/IRateSelection.h \
	$(INET_PROJ)/src/inet/linklayer/ieee80211/mac/IRx.h \
	$(INET_PROJ)/src/inet/linklayer/ieee80211/mac/IStatistics.h \
	$(INET_PROJ)/src/inet/linklayer/ieee80211/mac/ITx.h \
	$(INET_PROJ)/src/inet/linklayer/ieee80211/mac/ITxCallback.h \
	$(INET_PROJ)/src/inet/linklayer/ieee80211/mac/IUpperMac.h \
	$(INET_PROJ)/src/inet/linklayer/ieee80211/mac/Ieee80211Frame_m.h \
	$(INET_PROJ)/src/inet/linklayer/ieee80211/mac/Ieee80211Mac.h \
	$(INET_PROJ)/src/inet/linklayer/ieee80211/mac/MacParameters.h \
	$(INET_PROJ)/src/inet/linklayer/ieee80211/mac/MacPlugin.h \
	$(INET_PROJ)/src/inet/linklayer/ieee80211/mac/MacUtils.h \
	$(INET_PROJ)/src/inet/mobility/contract/IMobility.h \
	$(INET_PROJ)/src/inet/networklayer/common/InterfaceEntry.h \
	$(INET_PROJ)/src/inet/networklayer/common/InterfaceToken.h \
	$(INET_PROJ)/src/inet/networklayer/common/L3Address.h \
	$(INET_PROJ)/src/inet/networklayer/common/ModuleIdAddress.h \
	$(INET_PROJ)/src/inet/networklayer/common/ModulePathAddress.h \
	$(INET_PROJ)/src/inet/networklayer/contract/IInterfaceTable.h \
	$(INET_PROJ)/src/inet/networklayer/contract/ipv4/IPv4Address.h \
	$(INET_PROJ)/src/inet/networklayer/contract/ipv6/IPv6Address.h \
	$(INET_PROJ)/src/inet/physicallayer/base/packetlevel/PhysicalLayerDefs.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/bitlevel/ISignalAnalogModel.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IAntenna.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IArrival.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IInterference.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IListening.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IListeningDecision.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IModulation.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/INoise.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IPhysicalLayer.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IPhysicalLayerFrame.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IPrintableObject.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IRadio.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IRadioFrame.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IRadioSignal.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IReceiver.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IReception.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IReceptionDecision.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IReceptionResult.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/ISNIR.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/ITransmission.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/ITransmitter.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/RadioControlInfo_m.h \
	$(INET_PROJ)/src/inet/physicallayer/ieee80211/mode/IIeee80211Mode.h \
	$(INET_PROJ)/src/inet/physicallayer/ieee80211/mode/Ieee80211Band.h \
	$(INET_PROJ)/src/inet/physicallayer/ieee80211/mode/Ieee80211Channel.h \
	$(INET_PROJ)/src/inet/physicallayer/ieee80211/mode/Ieee80211ModeSet.h \
	$(INET_PROJ)/src/inet/physicallayer/ieee80211/packetlevel/Ieee80211ControlInfo_m.h
$O/dfra/mgmt/DfraAgentSTA.o: dfra/mgmt/DfraAgentSTA.cc \
	dfra/mgmt/DfraAgentSTA.h \
	$(INET_PROJ)/src/inet/common/Compat.h \
	$(INET_PROJ)/src/inet/common/INETDefs.h \
	$(INET_PROJ)/src/inet/common/INETUtils.h \
	$(INET_PROJ)/src/inet/common/InitStages.h \
	$(INET_PROJ)/src/inet/common/ModuleAccess.h \
	$(INET_PROJ)/src/inet/common/NotifierConsts.h \
	$(INET_PROJ)/src/inet/common/lifecycle/ILifecycle.h \
	$(INET_PROJ)/src/inet/features.h \
	$(INET_PROJ)/src/inet/linklayer/common/Ieee802Ctrl_m.h \
	$(INET_PROJ)/src/inet/linklayer/common/MACAddress.h \
	$(INET_PROJ)/src/inet/linklayer/ieee80211/mac/Ieee80211Frame_m.h \
	$(INET_PROJ)/src/inet/linklayer/ieee80211/mgmt/Ieee80211MgmtFrames_m.h \
	$(INET_PROJ)/src/inet/linklayer/ieee80211/mgmt/Ieee80211Primitives_m.h \
	$(INET_PROJ)/src/inet/networklayer/common/InterfaceEntry.h \
	$(INET_PROJ)/src/inet/networklayer/common/InterfaceTable.h \
	$(INET_PROJ)/src/inet/networklayer/common/InterfaceToken.h \
	$(INET_PROJ)/src/inet/networklayer/common/L3Address.h \
	$(INET_PROJ)/src/inet/networklayer/common/ModuleIdAddress.h \
	$(INET_PROJ)/src/inet/networklayer/common/ModulePathAddress.h \
	$(INET_PROJ)/src/inet/networklayer/contract/IInterfaceTable.h \
	$(INET_PROJ)/src/inet/networklayer/contract/ipv4/IPv4Address.h \
	$(INET_PROJ)/src/inet/networklayer/contract/ipv6/IPv6Address.h
$O/dfra/mgmt/DfraMgmtAP.o: dfra/mgmt/DfraMgmtAP.cc \
	dfra/mgmt/DfraMgmtAP.h \
	$(INET_PROJ)/src/inet/common/Compat.h \
	$(INET_PROJ)/src/inet/common/DelayedInitializer.h \
	$(INET_PROJ)/src/inet/common/INETDefs.h \
	$(INET_PROJ)/src/inet/common/INETMath.h \
	$(INET_PROJ)/src/inet/common/IVisitor.h \
	$(INET_PROJ)/src/inet/common/InitStages.h \
	$(INET_PROJ)/src/inet/common/ModuleAccess.h \
	$(INET_PROJ)/src/inet/common/NotifierConsts.h \
	$(INET_PROJ)/src/inet/common/Units.h \
	$(INET_PROJ)/src/inet/common/geometry/base/GeometricObjectBase.h \
	$(INET_PROJ)/src/inet/common/geometry/base/ShapeBase.h \
	$(INET_PROJ)/src/inet/common/geometry/common/Coord.h \
	$(INET_PROJ)/src/inet/common/geometry/common/EulerAngles.h \
	$(INET_PROJ)/src/inet/common/geometry/common/Rotation.h \
	$(INET_PROJ)/src/inet/common/geometry/object/LineSegment.h \
	$(INET_PROJ)/src/inet/common/lifecycle/ILifecycle.h \
	$(INET_PROJ)/src/inet/common/lifecycle/LifecycleOperation.h \
	$(INET_PROJ)/src/inet/common/lifecycle/NodeOperations.h \
	$(INET_PROJ)/src/inet/common/lifecycle/OperationalBase.h \
	$(INET_PROJ)/src/inet/common/mapping/Interpolation.h \
	$(INET_PROJ)/src/inet/common/mapping/MappingBase.h \
	$(INET_PROJ)/src/inet/common/mapping/MappingUtils.h \
	$(INET_PROJ)/src/inet/environment/common/Material.h \
	$(INET_PROJ)/src/inet/environment/contract/IMaterial.h \
	$(INET_PROJ)/src/inet/environment/contract/IMaterialRegistry.h \
	$(INET_PROJ)/src/inet/environment/contract/IPhysicalEnvironment.h \
	$(INET_PROJ)/src/inet/environment/contract/IPhysicalObject.h \
	$(INET_PROJ)/src/inet/features.h \
	$(INET_PROJ)/src/inet/linklayer/common/Ieee802Ctrl.h \
	$(INET_PROJ)/src/inet/linklayer/common/Ieee802Ctrl_m.h \
	$(INET_PROJ)/src/inet/linklayer/common/MACAddress.h \
	$(INET_PROJ)/src/inet/linklayer/contract/IMACProtocolControlInfo.h \
	$(INET_PROJ)/src/inet/linklayer/ieee80211/mac/Ieee80211Frame_m.h \
	$(INET_PROJ)/src/inet/linklayer/ieee80211/mgmt/Ieee80211MgmtAPBase.h \
	$(INET_PROJ)/src/inet/linklayer/ieee80211/mgmt/Ieee80211MgmtBase.h \
	$(INET_PROJ)/src/inet/linklayer/ieee80211/mgmt/Ieee80211MgmtFrames_m.h \
	$(INET_PROJ)/src/inet/mobility/contract/IMobility.h \
	$(INET_PROJ)/src/inet/networklayer/common/InterfaceEntry.h \
	$(INET_PROJ)/src/inet/networklayer/common/InterfaceToken.h \
	$(INET_PROJ)/src/inet/networklayer/common/L3Address.h \
	$(INET_PROJ)/src/inet/networklayer/common/ModuleIdAddress.h \
	$(INET_PROJ)/src/inet/networklayer/common/ModulePathAddress.h \
	$(INET_PROJ)/src/inet/networklayer/contract/IInterfaceTable.h \
	$(INET_PROJ)/src/inet/networklayer/contract/ipv4/IPv4Address.h \
	$(INET_PROJ)/src/inet/networklayer/contract/ipv6/IPv6Address.h \
	$(INET_PROJ)/src/inet/physicallayer/base/packetlevel/FlatRadioBase.h \
	$(INET_PROJ)/src/inet/physicallayer/base/packetlevel/FlatReceiverBase.h \
	$(INET_PROJ)/src/inet/physicallayer/base/packetlevel/FlatTransmitterBase.h \
	$(INET_PROJ)/src/inet/physicallayer/base/packetlevel/NarrowbandRadioBase.h \
	$(INET_PROJ)/src/inet/physicallayer/base/packetlevel/NarrowbandReceiverBase.h \
	$(INET_PROJ)/src/inet/physicallayer/base/packetlevel/NarrowbandTransmitterBase.h \
	$(INET_PROJ)/src/inet/physicallayer/base/packetlevel/PhysicalLayerBase.h \
	$(INET_PROJ)/src/inet/physicallayer/base/packetlevel/PhysicalLayerDefs.h \
	$(INET_PROJ)/src/inet/physicallayer/base/packetlevel/ReceiverBase.h \
	$(INET_PROJ)/src/inet/physicallayer/base/packetlevel/SNIRReceiverBase.h \
	$(INET_PROJ)/src/inet/physicallayer/base/packetlevel/TransmitterBase.h \
	$(INET_PROJ)/src/inet/physicallayer/common/packetlevel/Radio.h \
	$(INET_PROJ)/src/inet/physicallayer/common/packetlevel/RadioFrame.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/bitlevel/ISignalAnalogModel.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IAnalogModel.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IAntenna.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IArrival.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IBackgroundNoise.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IErrorModel.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IInterference.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IListening.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IListeningDecision.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IModulation.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/INoise.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IObstacleLoss.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IPathLoss.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IPhysicalLayer.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IPhysicalLayerFrame.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IPrintableObject.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IPropagation.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IRadio.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IRadioFrame.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IRadioMedium.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IRadioSignal.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IReceiver.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IReception.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IReceptionDecision.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IReceptionResult.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/ISNIR.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/ITransmission.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/ITransmitter.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/RadioControlInfo_m.h \
	$(INET_PROJ)/src/inet/physicallayer/ieee80211/mode/IIeee80211Mode.h \
	$(INET_PROJ)/src/inet/physicallayer/ieee80211/mode/Ieee80211Band.h \
	$(INET_PROJ)/src/inet/physicallayer/ieee80211/mode/Ieee80211Channel.h \
	$(INET_PROJ)/src/inet/physicallayer/ieee80211/mode/Ieee80211ModeSet.h \
	$(INET_PROJ)/src/inet/physicallayer/ieee80211/packetlevel/Ieee80211ControlInfo_m.h \
	$(INET_PROJ)/src/inet/physicallayer/ieee80211/packetlevel/Ieee80211Radio.h \
	$(INET_PROJ)/src/inet/physicallayer/ieee80211/packetlevel/Ieee80211ReceiverBase.h \
	$(INET_PROJ)/src/inet/physicallayer/ieee80211/packetlevel/Ieee80211TransmitterBase.h
$O/dfra/mgmt/DfraMgmtSTA.o: dfra/mgmt/DfraMgmtSTA.cc \
	dfra/mgmt/DfraMgmtSTA.h \
	$(INET_PROJ)/src/inet/common/Compat.h \
	$(INET_PROJ)/src/inet/common/DelayedInitializer.h \
	$(INET_PROJ)/src/inet/common/INETDefs.h \
	$(INET_PROJ)/src/inet/common/INETMath.h \
	$(INET_PROJ)/src/inet/common/INETUtils.h \
	$(INET_PROJ)/src/inet/common/IVisitor.h \
	$(INET_PROJ)/src/inet/common/InitStages.h \
	$(INET_PROJ)/src/inet/common/ModuleAccess.h \
	$(INET_PROJ)/src/inet/common/NotifierConsts.h \
	$(INET_PROJ)/src/inet/common/Units.h \
	$(INET_PROJ)/src/inet/common/geometry/base/GeometricObjectBase.h \
	$(INET_PROJ)/src/inet/common/geometry/base/ShapeBase.h \
	$(INET_PROJ)/src/inet/common/geometry/common/Coord.h \
	$(INET_PROJ)/src/inet/common/geometry/common/EulerAngles.h \
	$(INET_PROJ)/src/inet/common/geometry/common/Rotation.h \
	$(INET_PROJ)/src/inet/common/geometry/object/LineSegment.h \
	$(INET_PROJ)/src/inet/common/lifecycle/ILifecycle.h \
	$(INET_PROJ)/src/inet/common/mapping/Interpolation.h \
	$(INET_PROJ)/src/inet/common/mapping/MappingBase.h \
	$(INET_PROJ)/src/inet/common/mapping/MappingUtils.h \
	$(INET_PROJ)/src/inet/environment/common/Material.h \
	$(INET_PROJ)/src/inet/environment/contract/IMaterial.h \
	$(INET_PROJ)/src/inet/environment/contract/IMaterialRegistry.h \
	$(INET_PROJ)/src/inet/environment/contract/IPhysicalEnvironment.h \
	$(INET_PROJ)/src/inet/environment/contract/IPhysicalObject.h \
	$(INET_PROJ)/src/inet/features.h \
	$(INET_PROJ)/src/inet/linklayer/common/Ieee802Ctrl.h \
	$(INET_PROJ)/src/inet/linklayer/common/Ieee802Ctrl_m.h \
	$(INET_PROJ)/src/inet/linklayer/common/MACAddress.h \
	$(INET_PROJ)/src/inet/linklayer/contract/IMACProtocolControlInfo.h \
	$(INET_PROJ)/src/inet/linklayer/ieee80211/mac/Ieee80211Frame_m.h \
	$(INET_PROJ)/src/inet/linklayer/ieee80211/mgmt/Ieee80211MgmtBase.h \
	$(INET_PROJ)/src/inet/linklayer/ieee80211/mgmt/Ieee80211MgmtFrames_m.h \
	$(INET_PROJ)/src/inet/linklayer/ieee80211/mgmt/Ieee80211Primitives_m.h \
	$(INET_PROJ)/src/inet/mobility/contract/IMobility.h \
	$(INET_PROJ)/src/inet/networklayer/common/InterfaceEntry.h \
	$(INET_PROJ)/src/inet/networklayer/common/InterfaceToken.h \
	$(INET_PROJ)/src/inet/networklayer/common/L3Address.h \
	$(INET_PROJ)/src/inet/networklayer/common/ModuleIdAddress.h \
	$(INET_PROJ)/src/inet/networklayer/common/ModulePathAddress.h \
	$(INET_PROJ)/src/inet/networklayer/contract/IInterfaceTable.h \
	$(INET_PROJ)/src/inet/networklayer/contract/ipv4/IPv4Address.h \
	$(INET_PROJ)/src/inet/networklayer/contract/ipv6/IPv6Address.h \
	$(INET_PROJ)/src/inet/physicallayer/base/packetlevel/PhysicalLayerDefs.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/bitlevel/ISignalAnalogModel.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IAnalogModel.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IAntenna.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IArrival.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IBackgroundNoise.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IInterference.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IListening.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IListeningDecision.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IModulation.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/INoise.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IObstacleLoss.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IPathLoss.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IPhysicalLayer.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IPhysicalLayerFrame.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IPrintableObject.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IPropagation.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IRadio.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IRadioFrame.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IRadioMedium.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IRadioSignal.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IReceiver.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IReception.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IReceptionDecision.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/IReceptionResult.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/ISNIR.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/ITransmission.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/ITransmitter.h \
	$(INET_PROJ)/src/inet/physicallayer/contract/packetlevel/RadioControlInfo_m.h \
	$(INET_PROJ)/src/inet/physicallayer/ieee80211/mode/IIeee80211Mode.h \
	$(INET_PROJ)/src/inet/physicallayer/ieee80211/mode/Ieee80211Band.h \
	$(INET_PROJ)/src/inet/physicallayer/ieee80211/mode/Ieee80211Channel.h \
	$(INET_PROJ)/src/inet/physicallayer/ieee80211/mode/Ieee80211ModeSet.h \
	$(INET_PROJ)/src/inet/physicallayer/ieee80211/packetlevel/Ieee80211ControlInfo_m.h

