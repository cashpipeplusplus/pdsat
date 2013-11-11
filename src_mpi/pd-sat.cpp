// +---------------------------------------------------------------------------+
// | PD-SAT: Parallel Distributed SAT                                          }
// | MPI-realization of packet D-SAT for SAT-problem solving                   |                         
// +---------------------------------------------------------------------------+
// | Institute of System Dynamics and Control Theory SB RAS                    |   
// +---------------------------------------------------------------------------+
// | Author: Zaikin Oleg <oleg.zaikin@icc.ru>                                  |
// +---------------------------------------------------------------------------+

#include "mpi_base.h"
#include "mpi_solver.h"
#include "mpi_predicter.h"

struct Flags
{
	int solver_type;
	int sort_type;
	int koef_val;
	string schema_type; 
	int predict_from; 	
	int predict_to;
	int proc_count; 
	int poly_mod;
	int core_len; 
	bool IsPredict;	
	int cnf_in_set_count;
	double start_activity; 			
	bool IsConseq;		
	int best_lower_bound; 
	int upper_bound;
	int pb_mode;
	int exch_activ;			
	int verbosity;
	int check_every_conflict;
	int deep_predict;
	int deep_diff_decomp_set_count;
	int max_var_deep_predict;
	double start_temperature_koef;
	double point_admission_koef;
	int ts_strategy;
	int max_sat_problems;
	bool IsFirstStage;
	int max_L2_hamming_distance;
	bool IsSolveAll;
	double max_solving_time;
	int max_nof_restarts;
	int skip_tasks;
	string rslos_table_name;
};

// prototypes
//---------------------------------------------------------
bool GetInputFlags( int &argc, char **&argv, Flags &myflags );
const char* hasPrefix( const char* str, const char* prefix );
string hasPrefix_String( string str, string prefix );
void WriteUsage( );
void TestPredict( );
void TestDeepPredict( );
void TestSolve( );

//---------------------------------------------------------
int main( int argc, char** argv )
{
// main procedure
	int full_mask_var_count;
	char *input_cnf_name;

#ifdef _DEBUG
	TestSolve( );
	//TestDeepPredict( );
	//TestSolveQAP( );
	//TestPredict( );
	//TestPBSolve( );
	//TestSATSolve( );
#endif

	Flags myflags;

	// check input flags -solver -sort -koef -schema -predict -predict_from -predict_to
	if ( !GetInputFlags( argc, argv, myflags ) )
	{ printf( "\n Error in GetInputFlags" ); WriteUsage( ); return 1; }
	
	if ( myflags.IsConseq ) {
		//cout << endl << "before MPI_ConseqSolve solver_type is " << solver_type;
		//cout << endl << "before MPI_ConseqSolve solver_type is " << core_len;
		MPI_Solver mpi_s;
		if ( myflags.solver_type != -1 )
			mpi_s.solver_type = myflags.solver_type;
		if ( myflags.core_len != -1 )
			mpi_s.core_len = myflags.core_len;
		mpi_s.verbosity = myflags.verbosity;
		mpi_s.MPI_ConseqSolve( argc, argv );
		cout << endl << "Correct end of MPI_ConseqSolve";
		return 0;
	}

	if ( !(myflags.IsConseq) ) {
		// check count of input parameters
		if ( argc < 2 ) {
			std :: cout << "\n argc must be >= 2" << endl;
			WriteUsage( );
			return 1;
		}
		input_cnf_name = argv[1]; // get name of file with input CNF
	}
	
	if ( myflags.IsPredict ) {
		MPI_Predicter mpi_p;
		mpi_p.input_cnf_name          = input_cnf_name; // for C dminisat
		
		if ( myflags.solver_type != -1 )
			mpi_p.solver_type         = myflags.solver_type;
		if ( myflags.koef_val != -1 )
			mpi_p.koef_val            = myflags.koef_val;
		if ( myflags.schema_type != "" )
			mpi_p.schema_type         = myflags.schema_type;
		if ( myflags.predict_from != -1 )
			mpi_p.predict_from        = myflags.predict_from;
		if ( myflags.predict_to != -1 )
			mpi_p.predict_to          = myflags.predict_to;
		if ( myflags.proc_count != -1 )
			mpi_p.proc_count          = myflags.proc_count;
		if ( myflags.core_len != -1 )
			mpi_p.core_len            = myflags.core_len;
		if ( myflags.cnf_in_set_count != -1 )
			mpi_p.cnf_in_set_count    = myflags.cnf_in_set_count;
		if ( myflags.max_sat_problems != -1 )
			mpi_p.max_sat_problems    = myflags.max_sat_problems;
		if ( myflags.start_activity != -1 )
			mpi_p.start_activity = myflags.start_activity;
		if ( myflags.verbosity > 0 )
			mpi_p.verbosity = myflags.verbosity;
		if ( myflags.deep_diff_decomp_set_count > 0 )
			mpi_p.deep_diff_decomp_set_count = myflags.deep_diff_decomp_set_count;
		if ( myflags.max_var_deep_predict > 0 )
			mpi_p.max_var_deep_predict = myflags.max_var_deep_predict;
		if ( myflags.deep_predict > 0 )
			mpi_p.deep_predict = myflags.deep_predict;
		if ( myflags.start_temperature_koef > 0 )
			mpi_p.start_temperature_koef = myflags.start_temperature_koef;
		if ( myflags.point_admission_koef > 0 )
			mpi_p.point_admission_koef = myflags.point_admission_koef;
		if ( myflags.ts_strategy != -1 )
			mpi_p.ts_strategy = myflags.ts_strategy;
		if ( myflags.max_L2_hamming_distance > 0 )
			mpi_p.max_L2_hamming_distance = myflags.max_L2_hamming_distance;
		if ( myflags.max_solving_time > 0 )
			mpi_p.max_solving_time = myflags.max_solving_time;
		mpi_p.IsFirstStage = myflags.IsFirstStage;

		// Predict compute cost
		if ( !mpi_p.MPI_Predict( argc, argv ) ) {
			printf( "\n Error in MPI_Predict" );
			return 1;
		}
	}
	else { // Solve SAT-problem
		// get count of vars for splitting
		if ( argc == 3 ) {
			full_mask_var_count = atoi( argv[2] );
			if ( full_mask_var_count < 0 ) {
				printf( "\n Error. full_mask_var_count < 0" );
				WriteUsage( );
				return 1;
			}
		}
		else 
			full_mask_var_count = 31; // default

		MPI_Solver mpi_s;
		mpi_s.input_cnf_name = input_cnf_name;
		
		if ( full_mask_var_count != -1 )
			mpi_s.full_mask_var_count = (unsigned)full_mask_var_count;

		if ( myflags.solver_type != -1 )
			mpi_s.solver_type = myflags.solver_type;
		if ( myflags.sort_type != -1 )
			mpi_s.sort_type   = myflags.sort_type;
		if ( myflags.koef_val != -1 )
			mpi_s.koef_val    = myflags.koef_val;
		if ( myflags.schema_type != "" )
			mpi_s.schema_type = myflags.schema_type;
		if ( myflags.core_len != -1 )
			mpi_s.core_len    = myflags.core_len;
		if ( myflags.start_activity != -1 )
			mpi_s.start_activity = myflags.start_activity;
		if ( myflags.check_every_conflict != -1 )
			mpi_s.check_every_conflict = myflags.check_every_conflict;
		if ( myflags.exch_activ != -1 )
			mpi_s.exch_activ = myflags.exch_activ;
		if ( myflags.max_solving_time > 0 )
			mpi_s.max_solving_time = myflags.max_solving_time;
		if ( myflags.max_nof_restarts > 0 )
			mpi_s.max_nof_restarts = myflags.max_nof_restarts; 
		mpi_s.rslos_table_name = myflags.rslos_table_name;

		mpi_s.verbosity		   = myflags.verbosity;
		mpi_s.IsConseq         = myflags.IsConseq;
		mpi_s.best_lower_bound = myflags.best_lower_bound;
		mpi_s.upper_bound	   = myflags.upper_bound;
		mpi_s.PB_mode	       = myflags.pb_mode;
		mpi_s.IsSolveAll       = myflags.IsSolveAll;

		if ( !mpi_s.MPI_Solve( argc, argv ) ) {
			printf( "\n Error in MPI_Solve" );
			return 1;
		}
		mpi_s.~MPI_Solver( );
	}

	return 0;
}

//---------------------------------------------------------
void WriteUsage( )
{
// Write info about usage
	std :: cout << 
	"\n USAGE: p-sat [options] <input_cnf> <split_var_count>"
	"\n options::"
	"\n   -solver = { dm, m2_orig, m2_mod }, -s = -solver"
	"\n	    dm		     - mod of minisat 1.14 for (9+11+11) schema,"
	"\n	    m2_orig		 - origignal minisat 2.0"
	"\n		m2_mod		 - modified minisat 2.0 on C++ (by constants changing)"
	"\n		m2.2		 - minisat 2.2"
	"\n   -sort = { 0, 1, 2 }"
	"\n	    0			 - no sort,"
	"\n	    1			 - only sort of tasks,"
	"\n		2			 - only sort of vectors in solving,"
	"\n		3			 - sort of tasks and sort of vectors"
	"\n   -koef = { 1, 2, ... }"
	"\n		coefficient for tasks count calculating"
	"\n	  -schema = { first, missone, firstandlast }"
	"\n		0 = first        - 1, 2, 3, ..."
	"\n		1 = missone      - 1, 3, 5, ..."
	"\n		2 = literal count"
	"\n		3 = jeroslaw-wang"
	"\n     4 = implicant count"
	"\n     a5 = 31 vars 1..9, 20..30, 42..52"
	"\n     rslos_end = last cells of of rslos"
	"\n   -predict_from  - left bound of predicting"
	"\n   -predict_to    - right bound of predicting"
	"\n   -cnf_in_set    - count of CNFs in set"
	"\n   -proc_count    - count of cores for predicting"
	"\n   -core          - count of core vars for dm and m2mod (by default = 64)"
	"\n   -conseq        - conseq solve of cnf in current path"
	"\n   -file_assumptions = read assumptions from file"
	"\n   -verb = {0, 1, 2, 3}  - scale of output info"
	"\n   -solve_all     - solve all problems, even if SAT was found"
	"\n   -pb_mode = { 1, 2, 3 }"
	"\n	    1 - pb ineq mode, decompose by input vars"  	
	"\n	    2 - pb eq mode, decompose by values (f(x)=y) of obj func"  	
	"\n	    3 - pb ineq mode, decompose by domains (f(x)<y) of values of obj func" 
	"\n   -best_lower_bound - lower known bound for PB problem"
	"\n   -upper_bound   - upper known bound for PB problem"
	"\n   -exch_activ    - for PB QAP - scale of message passing"
	"\n     0 - no help by other subproblems"
	"\n     1 - when subproblem done it's record is sending to others"
	"\n     2 - when iteration on every subproblem done it's record is sending on others"
	"\n   -check_every_conflict   - frequency of checking stop-messages"
    "\n   *** deep predict ***"
	"\n   -deep_predict - (integer) mode of deep predict. 1 - fixed neighborhood of sets for every current best point. "
	"\n			2 - new neighborhood for every new best point"
	"\n   -diff - (integer) amount of different sets in particular dimension of decomposition"
	"\n   -max_var_deep - (float) koefficient (0,1). koef*predict_to = msx amount of variables for changing"
    "\n   -ts_strategy - way of choosing new unchecked area";
	"\n   -max_sat_problems - max count of sat problems in iteration of prediction";
	"\n   -max_L2_hamming_distance - max distance of point from L2 from current point";
	"\n   -no_first_stage - disable first stage mode (checking all areas until power of dec. set increase)";
	"\n   -max_solving_time - max time in seconds for splving particular SAT subproblem";
	"\n   -rslos_table - file name to table with possible values of rslos";
	"\n   -max_nof_restarts - maximum number of restarts";
	"\n   -skip_tasks - count of already solved tasks";
}

//---------------------------------------------------------
bool hasPrefix_String( string str, string prefix, string &value )
{
	int found = str.find( prefix );
	if ( found != -1 ) {
		value = str.substr( found + prefix.length( ) );
		return true;
	}
	return false;
}

//---------------------------------------------------------
bool GetInputFlags( int &argc, char **&argv, Flags &myflags )
{
// Get input keys if such exists 
	int i, k;
	bool IsSchemaFixed = false;
	stringstream sstream;
	string argv_string,
		   value;
	// default values
	myflags.solver_type			= -1;
	myflags.sort_type			= -1;
	myflags.koef_val			= -1,
	myflags.schema_type			= "";
	myflags.poly_mod			= -1;
	myflags.proc_count			= -1;
	myflags.predict_from		= -1;
	myflags.predict_to			= -1; 
	myflags.start_activity      = -1;
	myflags.core_len			= -1;
	myflags.IsConseq            = false;
	myflags.best_lower_bound	= -1;
	myflags.upper_bound			= -1;
	myflags.pb_mode			    = 1;
	myflags.exch_activ			= 1;
	myflags.verbosity			= -1;
	myflags.core_len            = 0;
	myflags.check_every_conflict = -1;
	myflags.deep_predict = -1;
	myflags.cnf_in_set_count = -1;
	myflags.start_temperature_koef = -1;
	myflags.point_admission_koef = -1;
	myflags.verbosity = -1;
	myflags.deep_diff_decomp_set_count = -1;
	myflags.max_var_deep_predict = -1;
	myflags.ts_strategy = -1;
	myflags.max_sat_problems = -1;
	myflags.IsFirstStage = true;
	myflags.max_L2_hamming_distance = -1;
	myflags.IsSolveAll = false;
	myflags.max_solving_time = 0;
	myflags.rslos_table_name = "";
	myflags.max_nof_restarts = 0;
	myflags.skip_tasks = 0;

	k = 0;

	// check every input parameters for flag existing
    for ( i = 0; i < argc; i++ ) {
		sstream.str( "" );
		sstream.clear();
		sstream << argv[i];
		argv_string = sstream.str( );
		if ( ( hasPrefix_String( argv_string, "-from=",            value ) ) ||
		     ( hasPrefix_String( argv_string, "-predict_from=",    value ) ) )
		{
			myflags.predict_from = atoi( value.c_str( ) );
			if ( myflags.predict_from < 0 )
			{ cout << "Error. predict_from is negative"; return false; }
		}
		else if ( ( hasPrefix_String( argv_string, "-to=",          value ) ) ||
		          ( hasPrefix_String( argv_string, "-predict_to=",  value ) ) )
		{
			myflags.predict_to = atoi( value.c_str( ) );
			if ( myflags.predict_to < 1 )
			{ cout << "Error. predict_to < 1 "; return false; }
		}
		else if ( ( hasPrefix_String( argv_string, "-proc=",        value ) ) ||
			      ( hasPrefix_String( argv_string, "-proc_count=",  value ) ) )
		{
			myflags.proc_count = atoi( value.c_str( ) );
			if ( myflags.proc_count < 1 )
			{ cout << "Error. proc_count is negative"; return false; }
		}
		else if ( ( hasPrefix_String( argv_string, "-rslos_table=",      value ) ) ||
				  ( hasPrefix_String( argv_string, "-rslos_table_name=", value ) ) )
			myflags.rslos_table_name = value;
		else if ( hasPrefix_String( argv_string, "-conseq", value ) )
			myflags.IsConseq = true;
		else if ( hasPrefix_String( argv_string, "-no_first_stage", value ) )
			myflags.IsFirstStage = false;
		else if ( hasPrefix_String( argv_string, "-solve_all", value ) )
			myflags.IsSolveAll = true;
		else if ( hasPrefix_String( argv_string, "-max_L2_hamming_distance=", value ) )
			myflags.max_L2_hamming_distance = atoi( value.c_str( ) );
		else if ( hasPrefix_String( argv_string, "-skip_tasks=", value ) )
			myflags.skip_tasks = atoi( value.c_str( ) );
		else if ( hasPrefix_String( argv_string, "-max_nof_restarts=", value ) )
			myflags.max_nof_restarts = atoi( value.c_str( ) );
		else if ( hasPrefix_String( argv_string, "-max_solving_time=", value ) )
			myflags.max_solving_time = atof( value.c_str( ) );
		else if ( hasPrefix_String( argv_string, "-deep_predict=", value ) )  {
			myflags.deep_predict = atoi( value.c_str( ) );
			switch ( myflags.deep_predict ) {
				case 1 : // method 1
					myflags.max_var_deep_predict = 10;
					break;
				case 2 : // method 2
					myflags.max_var_deep_predict = 5;
					break;
				case 3 : // method 3
					myflags.max_var_deep_predict = 3;
					break;
				case 4 : // method 4 : local search with checking all area
					myflags.max_var_deep_predict = 3;
					break;
				case 5 : // method 5 : simulated annelaling
					myflags.max_var_deep_predict = 2; // Hamming distance == 2
					break;
				case 6 : // method 5 : local + simulating
					myflags.max_var_deep_predict = 1; // Hamming distance == 2
					break;
				default :
					cout << "***Warning. Incorrect myflags.deep_predict";
					myflags.max_var_deep_predict = 2;
					break;
			}
		}
		else if ( ( hasPrefix_String( argv_string, "-solver=",      value ) ) || 
				  ( hasPrefix_String( argv_string, "-s=",           value ) ) )
		{
			if ( ( value == "dm"       ) || 
				 ( value == "dminisat" ) )
			{
				myflags.solver_type = 1;
				if ( !IsSchemaFixed ) // if no key -schema
					myflags.schema_type = "0"; // default schema is first variables
			}
			else if ( value == "m2.2" )
				myflags.solver_type = 4;
			else
                cout << "ERROR! unknown solver " << value;
		}
		else if ( hasPrefix_String( argv_string, "-sort=",       value ) )
		{
			myflags.sort_type = atoi( value.c_str( ) );
			if ( myflags.sort_type > 3 ) {
				cout << "Warning! unknown sort " << value << " changed to 0";
				myflags.sort_type = 0;
			}
		}
		else if ( hasPrefix_String( argv_string, "-koef=",       value ) ) {
			myflags.koef_val = atoi( value.c_str( ) );
			if ( myflags.koef_val < 1 ) {
				myflags.koef_val = 1;
				cout << "koef_val was changed to 1";
			}
		}
		else if ( ( hasPrefix_String( argv_string, "-core=",     value ) ) ||
			      ( hasPrefix_String( argv_string, "-core_len=", value ) ) )
		{
			myflags.core_len = atoi( value.c_str( ) );
			if ( myflags.core_len < 0 ) {
				myflags.core_len = 0;
				cout << "Error. core_len < 0, changed to 0";
			}
			if ( myflags.core_len > MAX_CORE_LEN ) { // check < MAX_CORE_LEN
				myflags.core_len = MAX_CORE_LEN;
				cout << "Error. core_len > MAX_CORE_LEN, changed to MAX_CORE_LEN";
			}
		}
		else if ( ( hasPrefix_String( argv_string, "-cnf_in_set=",       value ) ) ||
			      ( hasPrefix_String( argv_string, "-cnf_in_set_count=", value ) ) )
		{
			myflags.cnf_in_set_count = atoi( value.c_str( ) );
			if ( myflags.cnf_in_set_count < 0 ) {
				myflags.cnf_in_set_count = 64;
				cout << "cnf_in_set_count was changed to " << myflags.cnf_in_set_count;
			}
			if ( myflags.cnf_in_set_count > ( 1 << MAX_STEP_CNF_IN_SET ) ) {
				myflags.cnf_in_set_count = ( 1 << MAX_STEP_CNF_IN_SET );
				cout << "cnf_in_set_count was changed to " << myflags.cnf_in_set_count;
			}
		}
		else if ( hasPrefix_String( argv_string, "-schema=",              value ) ) {
			IsSchemaFixed = true;
			myflags.schema_type = value;
		}
		else if ( hasPrefix_String( argv_string, "-pb_mode=",             value ) ) {
			myflags.pb_mode = atoi( value.c_str( ) );
			if ( myflags.pb_mode > 3 ) myflags.pb_mode = 1;
		}
		else if ( hasPrefix_String( argv_string, "-ts_strategy=",          value ) )
			myflags.ts_strategy = atoi( value.c_str( ) );
		else if ( hasPrefix_String( argv_string, "-max_sat_problems=",     value ) )
			myflags.max_sat_problems = atoi( value.c_str( ) );
		else if ( hasPrefix_String( argv_string, "-start_activity=",       value ) )
			myflags.start_activity = atof( value.c_str( ) );
		else if ( hasPrefix_String( argv_string, "-exch_activ=",           value ) )
			myflags.exch_activ = atoi( value.c_str( ) );
		else if ( hasPrefix_String( argv_string, "-check_every_conflict=", value ) )
			myflags.check_every_conflict = atoi( value.c_str( ) );
		else if ( hasPrefix_String( argv_string, "-verb=",				   value ) )
			myflags.verbosity = atoi( value.c_str( ) );
		else if ( hasPrefix_String( argv_string, "-blb=",                  value ) )
			myflags.best_lower_bound = atoi( value.c_str( ) );
		else if ( hasPrefix_String( argv_string, "-ub=",                   value ) )
			myflags.upper_bound = atoi( value.c_str( ) );
		// deep predict
		else if ( hasPrefix_String( argv_string, "-diff=",				   value ) )
			myflags.deep_diff_decomp_set_count = atoi( value.c_str( ) );
		else if ( hasPrefix_String( argv_string, "-max_var_deep=",		   value ) )
			myflags.max_var_deep_predict = atoi( value.c_str( ) );
		else if ( hasPrefix_String( argv_string, "-start_temperature_koef=", value ) )
			myflags.start_temperature_koef = atof( value.c_str( ) );
		else if ( hasPrefix_String( argv_string, "-point_admission_koef=",	 value ) )
			myflags.point_admission_koef = atof( value.c_str( ) );
		else if ( ( argv_string == "-h"     ) || 
			      ( argv_string == "-help"  ) || 
				  ( argv_string == "--help" ) )
			WriteUsage( );
		else if ( argv_string[0] == '-' )
            cout << "ERROR! unknown flag " << argv_string;
		else
            argv[k++] = argv[i]; // skip flag arguments
    }
    argc = k;

	// if all keys needed for predicting exists then start predicting
	if ( ( myflags.predict_to > 0 ) || 
		 ( myflags.deep_predict > 0 ) ||
		 ( myflags.cnf_in_set_count > 0 )
	   )
	{
		myflags.IsPredict = true;
		/*cout << "myflags.IsPredict " << myflags.IsPredict << endl;
		cout << "myflags.start_temperature_koef " << myflags.start_temperature_koef << endl;
		cout << "myflags.point_admission_koef " << myflags.point_admission_koef << endl;
		cout << endl;*/
		if ( ( myflags.predict_to > MAX_CORE_LEN ) || ( myflags.predict_from > myflags.predict_to ) )
		{
			std :: cout << "\n Error. predict_from < 0 || predict_to > MAX_REDICT_TO ||" 
						<< " predict_from > predict_to" << endl;
			return false;
		}
	}
	else myflags.IsPredict = false;

	/*
	cout << "\npredict_from "  << predict_from;
	cout << "\npredict_to "    << predict_to;
	cout << "\nproc_count "    << proc_count;
	cout << "\nIsPredict "     << IsPredict;
	*/

	return true;
}

//---------------------------------------------------------
void TestPredict( )
{
// Testing predicter
	cout << "*** DEBUG MODE" << endl;
	unsigned i;
	MPI_Predicter mpi_p;

	mpi_p.input_cnf_name = "D:\\ForCopy\\a5_144_1_64.cnf";
	//mpi_s.input_cnf_name = "D:\\ForCopy\\test3.cnf";

	vec< vec<Lit> > dummy_vec;
	unsigned part_mask[FULL_MASK_LEN];
	unsigned full_mask[FULL_MASK_LEN];
	unsigned value[FULL_MASK_LEN];
	for ( unsigned i = 0; i < FULL_MASK_LEN; i++ ) {
		part_mask[i] = 0;
		full_mask[i] = 0;
		value[i] = 0;
	}

	part_mask[1] = 3;
	full_mask[1] = 7;
	value[1] = 5;
	mpi_p.MakeAssignsFromMasks( full_mask, part_mask, value, dummy_vec );

	cout << "dummy_vec" << endl;
	for ( int i = 0; i < dummy_vec.size(); i++ ) {
		for ( int j=0; j < dummy_vec[i].size(); j++ )
			cout << dummy_vec[i][j].x << " ";
		cout << endl;
	}

	mpi_p.solver_type  = 0;
	mpi_p.schema_type  = "10";
	mpi_p.core_len     = 64;
	mpi_p.predict_from = 25;
	mpi_p.predict_to   = 31;
	mpi_p.cnf_in_set_count = 4;

	mpi_p.ReadVarCount( );

	// create array for answer
	mpi_p.b_SAT_set_array = new int[mpi_p.var_count];
	for ( i = 0; i < mpi_p.var_count; i++ )
		mpi_p.b_SAT_set_array[i] = -1;

	mpi_p.decomp_set_count = mpi_p.predict_to - mpi_p.predict_from + 1;

	mpi_p.PrepareForPredict( );

	int process_sat_count = 0;
	double cnf_time_from_node = 0;
	
	/*
	for ( i = 2; i < mpi_p.decomp_set_count; i++ )
	{
		mpi_p.cnf_real_time_arr[i] = 100 * ( 1 << i );
		mpi_p.cnf_status_arr[i]    = 0;
	}
	*/

	mpi_p.GetPredict( );

	//mpi_p.WritePredictToFile( 0 );

	mpi_p.~MPI_Predicter( );
}

void TestSolve()
{
	char *input_cnf_name = "../src_common/tresh72_0.cnf";
	
	string rslos_table_name = "../src_common/bits_4_1/1010.txt";
	int process_sat_count = 0;
	unsigned int full_mask[FULL_MASK_LEN];
	unsigned int part_mask[FULL_MASK_LEN];
	unsigned int value[FULL_MASK_LEN];
	MPI_Solver mpi_s;
	mpi_s.input_cnf_name = input_cnf_name;
	//mpi_s.schema_type = "rslos_end";
	mpi_s.solver_type = 4;
	mpi_s.ReadIntCNF();
	mpi_s.MakeVarChoose();
	double cnf_time_from_node;
	int current_task_index = 0, current_obj_val;
	double solving_times[SOLVING_TIME_LEN];
	Solver *S;
	
	for ( unsigned i=0; i < FULL_MASK_LEN; i++ )
		full_mask[i] = part_mask[i] = 0;
	full_mask[0] = part_mask[0] = value[0] = 1;
	full_mask[1] = 1048575;
	part_mask[1] = 1023;

	mpi_s.all_tasks_count = 16;
	//mpi_s.verbosity = 2;
	mpi_s.SolverRun( S, full_mask, part_mask, value, process_sat_count, current_obj_val,
					 cnf_time_from_node, solving_times, current_task_index );
	int solver_type = 1;
	int core_len = 64;
	double corevars_activ_type = 1;
	int *b_SAT_set_array;
	int sort_type = 0;

	dminisat_solve( input_cnf_name, full_mask, part_mask, value,
				    solver_type, core_len, corevars_activ_type, 
					&process_sat_count, &b_SAT_set_array, sort_type, 
					&cnf_time_from_node, 0, 0 );
}

//---------------------------------------------------------
void TestDeepPredict( )
{
	cout << "*** DEBUG MODE" << endl;
	MPI_Predicter mpi_p;

	vector<unsigned> rand_arr; 
	unsigned rand_arr_len = 192; 
	unsigned max_rand_val = 56;

	mpi_p.MakeUniqueRandArr( rand_arr, rand_arr_len, max_rand_val );
	sort( rand_arr.begin(), rand_arr.end() );

	mpi_p.predict_to = 177;
	mpi_p.predict_from = 0;
	mpi_p.core_len = 177;
	mpi_p.cnf_in_set_count = 2000000;
	mpi_p.deep_predict = 6;
	mpi_p.max_var_deep_predict = 1;

	mpi_p.part_mask[0] = 3;
	mpi_p.part_mask[1] = 4846842;
	mpi_p.part_mask[2] = 315856;
	vector< vector<unsigned> > values_arr;

	mpi_p.GetInitPoint();
	values_arr.resize(10);
	for ( unsigned i = 0; i < 10; i++ )
		values_arr[i].resize(FULL_MASK_LEN);
	mpi_p.GetRandomValuesArray( 10, values_arr );

	mpi_p.schema_type = "bivium_Ending2"; 
	mpi_p.GetInitPoint( );

	unchecked_area ua;
	ua.center.resize( mpi_p.core_len );
	for ( unsigned i = 0; i < mpi_p.core_len; i ++ )
		ua.center.set(i);
	mpi_p.L2.push_back( ua );
	boost::dynamic_bitset<> bs = mpi_p.IntVecToBitset( mpi_p.core_len, mpi_p.var_choose_order );
	stringstream sstream;
	mpi_p.AddNewUncheckedArea( bs, sstream );
	mpi_p.current_unchecked_area.center = bs;
	mpi_p.cur_vars_changing = 1;
	mpi_p.IsFirstPoint = true;
	//mpi_p.schema_type = "bivium_Ending2"
	mpi_p.deep_predict_cur_var = mpi_p.predict_to;
	mpi_p.GetDeepPredictTasks( );
	mpi_p.GetDeepPredictTasks( );
	mpi_p.cur_vars_changing = 1;
}