echo -e "\033[31m##################### WARNING ########################\033[0m"
echo -e "\033[31m####         THIS IS DEVELOPMENT VERSION          ####\033[0m"
echo -e "\033[31m######################################################\033[0m"
echo ""

# Set up environment
echo "@@@@ Working Directory: `pwd`"
export SKNANO_HOME=`pwd`
export SKNANO_RUNLOG="/home/$USER/workspace/SKNanoRunlog"
export SKNANO_OUTPUT="/home/$USER/workspace/SKNanoOutput"

# root configuration

echo "@@@@ Not running on redhat 7, 8, or 9"
echo "@@@@ Assuming root is installed in conda environment 'nano'"
if conda info --envs | grep -q "\bnano\b"; then
    echo "Environment 'nano' found. Activating..."
    conda activate nano
else
    echo "Environment 'nano' not found. Creating..."
    # Assuming nano.yaml is in the same directory as this script
    conda env create -f nano_env.yaml
    echo "Environment 'nano' created. Activating..."
    conda activate nano
  fi
fi
echo "@@@@ ROOT path: $ROOTSYS"


export SKNANO_LIB=$SKNANO_HOME/lib
export SKNANO_VERSION="Run3UltraLegacy_v1"
export SKNANO_DATA=$SKNANO_HOME/data/$SKNANO_VERSION
mkdir -p $SKNANO_DATA

export SKNANO_BIN=$SKNANO_HOME/bin
export SKNANO_PYTHON=$SKNANO_HOME/python
export PATH=$SKNANO_PYTHON:$PATH
export PYTHONPATH=$PYTHONPATH:$SKNANO_PYTHON

export ROOT_INCLUDE_PATH=$ROOT_INCLUDE_PATH:$SKNANO_HOME/DataFormats/include
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$SKNANO_LIB
export DYLD_LIBRARY_PATH=$DYLD_LIBRARY_PATH:$SKNANO_LIB

# for LHAPDF
export LHAPDF_DATA_PATH=$SKNANO_HOME/external/lhapdf/data
echo "@@@@ reading LHPPDF from $LHAPDF_DATA_PATH"

# env for correctionlibs
export CORRECTION_LIBS=`correction config --libdir`
export CORRECTION_CMAKE_PREFIX=`correction config --cmake`

source $SKNANO_HOME/scripts/BashColorSets.sh

## Todo list ##
echo "@@@@ Printing To-Do Lists (Ctrl+C to skip)"
python scripts/PrintToDoLists.py
source $SKNANO_HOME/tmp/ToDoLists.sh
rm $SKNANO_HOME/tmp/ToDoLists.sh
