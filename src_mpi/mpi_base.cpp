#include "mpi_base.h"

#pragma warning( disable : 4996 )

const int    MAX_WORD_LENGTH			   = 64;
const int    MAX_LINE_LENGTH               = 524288;
const int    MAX_LINE_LENGTH_2             = 8192;
const int    MEDIUM_STRING_LEN             = 4096;
const double TRANSP_COAST                  = 0.000001;
const int    NUM_KEY_BITS                  = 64;

//=================================================================================================
// Constructor/Destructor:

MPI_Base :: MPI_Base( ) :
	sat_count            ( 0 ),
	corecount			 ( 0 ),
	solver_name          ( "" ),
	core_len             ( 0 ),
	koef_val             ( 4 ),
	schema_type          ( "" ),
	var_count            ( 0 ),
	clause_count         ( 0 ),
	full_mask_var_count  ( 0 ),
	start_activity		 ( 0 ),
	part_mask_var_count  ( 0 ),
	all_tasks_count		 ( 0 ),
	isConseq             ( false ),
	cnf_in_set_count     ( 100 ),
	verbosity			 ( 0 ),
	known_point_file_name ( "known_point" ),
	isSolveAll           ( false ),
	isPredict            ( false ),
	max_solving_time     ( 0 ),
	max_nof_restarts     ( 0 ),
	first_stream_var_index ( 0 ),
	te ( 0 ),
	known_bits ( 0 ),
	output_len ( 200 ),
	isMakeSatSampleAnyWay ( false ),
	isSolverSystemCalling ( false ),
	process_sat_count ( 0 ),
	known_vars_count ( 0 ),
	isPlainText (false),
	evaluation_type("time"),
	rank (0),
	total_start_time (0.0)
{
	for ( unsigned i = 0; i < FULL_MASK_LEN; i++ )
		full_mask[i] = part_mask[i] = mask_value[i] = 0;

	gen.seed( static_cast<unsigned>(std::time(0)) );
}

MPI_Base :: ~MPI_Base( )
{ }

// Make full_mask and part_mask for sending from order that is set by var choose array
//---------------------------------------------------------
bool MPI_Base :: GetMainMasksFromVarChoose( std::vector<int> &var_choose_order )
{
	if ( verbosity > 0 ) {
		std::cout << std::endl << "GetMainMasksFromVarChoose() start with var_choose_order " << std::endl;
		for ( unsigned i = 0; i < var_choose_order.size(); i++ )
			std::cout << var_choose_order[i] << " ";
		std::cout << std::endl;
		std::cout << "part_mask_var_count " << part_mask_var_count << std::endl;
		std::cout << std::endl;
	}

	// init masks every launch
	for ( unsigned i = 0; i < FULL_MASK_LEN; ++i )
		part_mask[i] = full_mask[i] = 0;
	
	unsigned cur_uint_ind, var_index;
	for ( unsigned i = 0; i < var_choose_order.size(); ++i ) {
		var_index = var_choose_order[i] - 1;
		cur_uint_ind = var_index / UINT_LEN;
		full_mask[cur_uint_ind] += 1 << ( var_index % UINT_LEN );
	}

	// get first full_mask_var_count vars from  array var_choose_order
	for ( unsigned i = 0; i < part_mask_var_count; ++i ) {
		var_index = var_choose_order[i] - 1;
		cur_uint_ind = var_index / UINT_LEN;
		part_mask[cur_uint_ind] += 1 << ( var_index % UINT_LEN );
	}
	
	if ( verbosity > 1 ) {
		std::cout << "made part_mask" << std::endl;
		for( unsigned i = 0; i < FULL_MASK_LEN; ++i )
			std::cout << part_mask[i] << " ";
		std::cout << std::endl;
		unsigned bit_count = 0;
		for ( unsigned j=0; j < FULL_MASK_LEN; ++j )
			bit_count += BitCount( part_mask[j] );
		std::cout << "part_mask bit_count " << bit_count << std::endl;
	}
	
	if ( verbosity > 1 )
		std::cout << "GetMainMasksFromVarChoose() end" << std::endl;
	
	return true;
}
/*
bool MPI_Base :: MakeAssignsFromFile( int current_task_index, unsigned long long before_binary_length, vec< vec<Lit> > &dummy_vec )
{
	if ( verbosity > 0 )
		std::cout << "MakeAssignsFromFile()" << std::endl;

	if ( current_task_index < 0 ) {
		std::cerr << "current_task_index < 0" << std::endl;
		return false;
	}
	
	if ( var_choose_order.size() == 0 ) {
		std::cerr << "var_choose_order.size() == 0 " << std::endl;
		return false;
	}
	
	// int rslos_num = 1;
	unsigned long long basic_batch_size = (unsigned long long)floor( (double)assumptions_count / (double)all_tasks_count );
	// calculate count of bathes with additional size (+1)
	unsigned long long batch_addit_size_count = assumptions_count - basic_batch_size*all_tasks_count;
	unsigned long long cur_batch_size = basic_batch_size;
	if ( (unsigned long long)current_task_index < batch_addit_size_count )
		cur_batch_size++;

	unsigned max_int = (1 << 31);
	if ( cur_batch_size > (unsigned long long)max_int ) {
		std::cerr << "cur_batch_size > (unsigned long long)max_int" << std::endl;
		return false;
	}
	// skip unuseful strings
	unsigned long long previous_problems_count = (unsigned long long)current_task_index*basic_batch_size;
	if ( (unsigned long long)current_task_index < batch_addit_size_count )
		previous_problems_count += (unsigned long long)current_task_index; // add some 1 to sum
	else
		previous_problems_count += batch_addit_size_count;
	
	std::string str, str1;
	//cout << "before_binary_length " << before_binary_length << std::endl;
	ifile.seekg( before_binary_length, ifile.beg ); // skip some bytes
	
	char *cc = new char[3];
	cc[2] = '\0';
	ifile.read(cc,2);
	std::stringstream sstream;
	unsigned header_value;
	sstream << cc;
	sstream >> header_value;
	delete[] cc;
	//ifile >> header_value; // read header in text mode
	if ( header_value != var_choose_order.size() ) {
		std::cerr << "header_value != var_choose_order.size()" << std::endl;
		std::cerr << header_value << " != " << var_choose_order.size() << std::endl;
		std::cerr << "cc " << cc << std::endl;
		return false;
	}
	
	// reading values from file
	dummy_vec.resize( (unsigned)cur_batch_size );
	boost::dynamic_bitset<> d_bitset;
	d_bitset.resize( var_choose_order.size() );
	int cur_var_ind;
	unsigned long long ul;
	unsigned long long byte_count = before_binary_length + 2 + sizeof(ul)*previous_problems_count;
	std::stringstream sstream_info;
	sstream_info << "current_task_index "      << current_task_index << std::endl;
	sstream_info << "all_tasks_count "         << all_tasks_count << std::endl;
	sstream_info << "previous_problems_count " << previous_problems_count << std::endl;
	sstream_info << "basic_batch_size "        << basic_batch_size  << std::endl;
	sstream_info << "cur_batch_size "          << cur_batch_size << std::endl;
	sstream_info << "byte_count "              << byte_count << std::endl;
	if ( verbosity > 0 )
		std::cout << sstream_info.str() << std::endl;
	ifile.clear();
	ifile.seekg( byte_count, ifile.beg ); // skip some bytes
	if ( ifile.fail() || ifile.eof() ) {
		std::cerr << "Error. ifile.fail() || ifile.eof()" << std::endl;
		std::cerr << sstream_info.str();
		return false;
	}
	for ( unsigned i=0; i < (unsigned)cur_batch_size; ++i ) {
		if ( !(ifile.read( (char*)&ul, sizeof(ul) ) ) ) {
			std::cerr << "Error. !ifile.read( (char*)&ul, sizeof(ul) )" << std::endl;
			std::cerr << sstream_info.str();
			return false;
		}
		UllongToBitset( ul, d_bitset );
		if ( d_bitset.size() != var_choose_order.size() ) {
			std::cerr << "d_bitset.size() != var_choose_order.size()" << std::endl;
			std::cerr << d_bitset.size() << " != " << var_choose_order.size() << std::endl;
			return false;
		}
		for ( unsigned j=0; j < var_choose_order.size(); ++j ) {
			cur_var_ind = var_choose_order[j] - 1;
			if ( d_bitset[j] == 1 )
				dummy_vec[i].push( mkLit( cur_var_ind ) );
			else 
				dummy_vec[i].push( ~mkLit( cur_var_ind ) );
		}
	}
	ifile.close();
	return true;
}*/

bool MPI_Base :: MakeAssignsFromMasks( unsigned *full_mask, 
									   unsigned *part_mask, 
									   unsigned *mask_value,
									   vec<Lit> &known_dummy,
									   vec< vec<Lit> > &dummy_vec )
{
    // convert masks to a vector of Literals
	unsigned variate_vars_count = 0, known_vars_count = 0;
	full_mask_var_count = 0;
	for ( unsigned i = 0; i < FULL_MASK_LEN; i++ ) {
		variate_vars_count  += BitCount( full_mask[i] ^ part_mask[i] );
		full_mask_var_count += BitCount( full_mask[i] );
	}
	known_vars_count = full_mask_var_count - variate_vars_count;
	
	unsigned mask;
	int cur_var_ind;
	
	// get literals which can be assigned once (from part_mask)
	for (unsigned i = 0; i < FULL_MASK_LEN; i++) {
		for (unsigned j = 0; j < UINT_LEN; j++) {
			mask = (1 << j);
			cur_var_ind = i * UINT_LEN + j;
			if (part_mask[i] & mask) // one common vector send by control process
				known_dummy.push((mask_value[i] & mask) ? mkLit(cur_var_ind) : ~mkLit(cur_var_ind) );
		}
	}

	if (known_dummy.size() != known_vars_count) {
		std::cerr << "known_dummy.size() != known_vars_count" << std::endl;
		std::cerr << known_dummy.size() << " != " << known_vars_count << std::endl;
#ifdef _MPI
		MPI_Abort( MPI_COMM_WORLD, 0 );
#endif
	}
	
	if (verbosity > 2)
		std::cout << "known_dummy.size() " << known_dummy.size() << std::endl;

	// determine the number of assumptions and their lengths
	int problems_count = 1 << variate_vars_count;
	dummy_vec.resize(problems_count);
	for (int i = 0; i < dummy_vec.size(); ++i)
		dummy_vec[i].resize(variate_vars_count);

	// get array of literals which must be checked
	for ( int lint = 0; lint < problems_count; lint++ ) {
		unsigned range_mask = 1;
		unsigned range_mask_ind = 1;
		unsigned index = 0;
		for ( unsigned i = 0; i < FULL_MASK_LEN; i++ ) {
			for ( unsigned j = 0; j < UINT_LEN; j++ ) {
				mask = ( 1 << j );
				cur_var_ind = i * UINT_LEN + j;
				if ( ( full_mask[i] & mask ) && !( part_mask[i] & mask ) ) { // set of vectors
					range_mask = 1 << range_mask_ind;
					range_mask_ind++;
					dummy_vec[lint][index++] = (lint & range_mask) ? mkLit(cur_var_ind) : ~mkLit(cur_var_ind);
				}
			}
		}
	}

	if (verbosity > 2)
		std::cout << "dummy_vec.size() " << dummy_vec.size() << std::endl;

	if (verbosity > 0)
		std::cout << "MakeAssignsFromMasks() done " << std::endl;

	return true;
}

// Get values for sending using order by var choose array
//---------------------------------------------------------
bool MPI_Base :: GetValuesFromVarChoose( unsigned &part_var_power )
{
	unsigned cur_uint_ind = 0;
	unsigned value_index;
	unsigned mask;
	unsigned mask2;
	for( unsigned lint = 0; lint < part_var_power; lint++ ) {
		value_index = 0;
		for( unsigned i = 0; i < FULL_MASK_LEN; i++ ) {
			for( unsigned j = 0; j < UINT_LEN; j++ ) {
				mask  = ( 1 << j );
				mask2 = 1 << value_index;
				if ( part_mask[i] & mask ) { // if part_mask bit is 1	
					if ( lint & mask2 ) // if bit is 1
						values_arr[lint][i] += mask;
					value_index++;
				}
			} // for( j = 0; j < UINT_LEN; j++ )
		}
	} // for( lint = 0; lint < part_var_power; lint++ )

	return true;
}

//---------------------------------------------------------
bool MPI_Base::getValuesFromIntegers(std::vector<std::vector<int>> cartesian_elements)
{
	unsigned mask;
	int cur_integer_index;
	unsigned cur_var_index = 0;
	unsigned k;
	
	for (unsigned cartesian_element_index = 0; cartesian_element_index < cartesian_elements.size(); cartesian_element_index++) {
		k = 0;
		cur_integer_index = cartesian_elements[cartesian_element_index][k];
		for (unsigned i = 0; i < FULL_MASK_LEN; i++) {
			if (k == cartesian_elements[cartesian_element_index].size())
				break;
			for (unsigned j = 0; j < UINT_LEN; j++) {
				cur_var_index = i*UINT_LEN + j;
				mask = (1 << j);
				if (cur_var_index == cur_integer_index) {
					values_arr[cartesian_element_index][i] += mask;
					k++;
					if (k == cartesian_elements[cartesian_element_index].size())
						break;
					cur_integer_index = cartesian_elements[cartesian_element_index][k];
				}
			} // for( j = 0; j < UINT_LEN; j++ )
		}
	} // for( lint = 0; lint < part_var_power; lint++ )

	return true;
}

//---------------------------------------------------------
bool MPI_Base :: makeStandardMasks( unsigned &part_var_power )
{		
	if ( !GetMainMasksFromVarChoose( var_choose_order ) ) { 
		std::cout << "Error in GetMainMasksFromVarChoose" << std::endl; 
		return false;
	}
	std::cout << "Correct end of GetMasksFromVarChoose" << std::endl;
		
	if ( !GetValuesFromVarChoose( part_var_power ) ) { 
		std::cout << "Error in GetValuesFromVarChoose" << std::endl; 
		return false; 
	}
	std::cout << "Correct end of GetValuesFromVarChoose" << std::endl;
	
	return true;
}

//---------------------------------------------------------
bool MPI_Base::makeIntegerMasks(std::vector<std::vector<int>> cartesian_elements)
{
	if (!GetMainMasksFromVarChoose(var_choose_order)) {
		std::cout << "Error in GetMainMasksFromVarChoose" << std::endl;
		return false;
	}
	std::cout << "Correct end of GetMasksFromVarChoose" << std::endl;

	if (!getValuesFromIntegers(cartesian_elements)) {
		std::cout << "Error in getValuesFromIntegers" << std::endl;
		return false;
	}
	
	std::cout << "Correct end of makeIntegerMasks" << std::endl;
	
	return true;
}

//---------------------------------------------------------
bool MPI_Base :: MakeVarChoose( )
{
// Make array var_choose_order with vars sorted by given rule
	std::string str;
	std::stringstream sstream;
	int val;

	std::cout << "start MakeVarChoose()" << std::endl;
	std::cout << "var_choose_order" << std::endl;
	for ( unsigned i = 0; i < var_choose_order.size(); i++ )
		std::cout << var_choose_order[i] << " ";
	std::cout << std::endl;
	
	// if file with decomposition set exists
	std::ifstream known_point_file( known_point_file_name.c_str() );
	if ( known_point_file.is_open() ) {
		getline( known_point_file, str );
		sstream << str;
		var_choose_order.resize( 0 );
		while ( sstream >> val )
			var_choose_order.push_back( val );
		full_mask_var_count = var_choose_order.size();
		sstream.str( "" ); sstream.clear( );
		known_point_file.close();
		schema_type = "known_point";
	}
	
	std::cout << "var_choose_order.size() " << var_choose_order.size() << std::endl;
	std::cout << "full_mask_var_count " << full_mask_var_count << std::endl;

	if ( ( var_choose_order.size() == 0 ) && ( schema_type == "" ) )
		schema_type = "0";
	// if got from known point file or from "c var_set..." then set was made already
	var_choose_order.resize( full_mask_var_count );
	
	if ( schema_type != "" ) {
		int k = 0;
		if ( schema_type == "0" ) { // == bivium_Beginning1
			for ( unsigned i = 0; i < full_mask_var_count; i++ )
				var_choose_order[i] = i + 1;
		}
		else if ( schema_type == "bivium_Beginning2" ) {
			for ( unsigned i = 0; i < full_mask_var_count; i++ )
				var_choose_order[i] = i + 84 + 1;
		}
		else if ( schema_type == "bivium_Beginning_halved" ) {
			for ( unsigned i = 0; i < full_mask_var_count / 2; i++ )
				var_choose_order[k++] = i + 1;
			for ( unsigned i = 0; i < full_mask_var_count / 2; i++ )	
				var_choose_order[k++] = i + 84 + 1;
		}
		else if ( schema_type == "bivium_Ending1" ) {
			for ( unsigned i = 0; i < full_mask_var_count; i++ )
				var_choose_order[i] = 84 - i;
		}
		else if ( schema_type == "bivium_Ending2" ) {
			for ( unsigned i = 0; i < full_mask_var_count; i++ )
				var_choose_order[i] = 177 - i;
		}
		else if ( schema_type == "bivium_Ending_halved" ) {
				for ( unsigned i = 0; i < full_mask_var_count / 2; i++ )
					var_choose_order[k++] = 84 - i;
				for ( unsigned i = 0; i < full_mask_var_count / 2; i++ )	
					var_choose_order[k++] = 177 - i;
		}		
		else if ( schema_type == "a5" ) {
			for ( unsigned i = 0; i < 9; i++ )
				var_choose_order[k++] = i + 1;
			for ( unsigned i = 0; i < 11; i++ )
				var_choose_order[k++] = i + 19 + 1;
			for ( unsigned i = 0; i < 11; i++ )
				var_choose_order[k++] = i + 41 + 1;
		}
	}

	std::cout << "final var_choose_order.size() " << var_choose_order.size() << std::endl;
	sort( var_choose_order.begin(), var_choose_order.end() );
	std::cout << "var_choose_order" << std::endl;
	for ( unsigned i = 0; i < var_choose_order.size(); i++ )
		std::cout << var_choose_order[i] << " ";
	std::cout << std::endl;

	return true;
}

//---------------------------------------------------------
bool MPI_Base :: ReadVarCount( )
{
// Reading actual vatiable count CNF from file (skipping line "p cnf ...").
	unsigned int current_clause_count = 0,
				 current_lit_count = 0,
				 line_str_len = 0,
				 current_var_count = 0,
				 i = 0, k = 0,
				 lit_positiv_val = 0;
	std::string line_str, word_str;
	bool IsUncorrectLine = false;
	
	// check file with main CNF
	std::ifstream main_cnf( input_cnf_name.c_str(), std::ios::in );
    if ( !main_cnf ) {
		std :: cerr << std::endl << "Error in opening of file with input CNF " 
			        << input_cnf_name << std::endl;
		return false;
	}

	// step 1 - get var_count and clause_count;
    while ( getline( main_cnf, line_str ) ) {
		if ( ( line_str[0] == 'p' ) || ( line_str[0] == 'c' ) )
			continue;

		else { // try to read line with clause 
			current_lit_count = 0;
			line_str = " " + line_str; // add space to line for correct work of parser
			for ( i = 0; i < line_str.length( ) - 1; i++ ) {
				IsUncorrectLine = false;
				if ( ( line_str[i] == ' ' ) && ( line_str[i + 1] != ' ' ) && 
					 ( line_str[i + 1] != '0' ) )
				{
					word_str = ""; // init string for cuttenr word from string
					k = i + 1; // k = index of first symbol in current word
					do {
						word_str += line_str[k];
						k++;
						if ( k == line_str.length( ) ) { // skip empty or uncorrect line
							/*std :: cout << "\n***In ReadVarCount skipped line " << 
								           line_str << std::endl;*/
							IsUncorrectLine = true;
							break;
						}
					} while ( line_str[k] != ' ' );

					if ( IsUncorrectLine )
						break;
					
					current_lit_count++;
					
					// convert value of literal to positiv int
					lit_positiv_val = abs( atoi( word_str.c_str( ) ) ); 
					if ( lit_positiv_val > current_var_count )
						current_var_count = lit_positiv_val;
				} // if ( ( line_str[i] == ' ' ) && ...
			} // for ( i = 0; i < line_str.length( ) - 1; i++ )

			if ( current_lit_count ) // if at least one lit exists then inc clause count
				current_clause_count++;
		}
	}

	var_count    = current_var_count;
	lit_count    = var_count * 2;
	clause_count = current_clause_count;

	clause_lengths.resize( clause_count );

	main_cnf.close( ); // reopen file
	main_cnf.clear( );
	main_cnf.open( input_cnf_name.c_str() );
	current_clause_count = 0;

	// step 2 - get arrays of lengths
	while ( getline( main_cnf, line_str ) ) {
		if ( ( line_str[0] == 'p' ) || ( line_str[0] == 'c' ) )
			continue;

		else { // try to read line with clause 
			current_lit_count = 0;
			line_str = " " + line_str; // add space to line for correct work of parser
			for ( i = 0; i < line_str.length( ) - 1; i++ ) {
				IsUncorrectLine = false;
				if ( ( line_str[i] == ' ' ) && ( line_str[i + 1] != ' ' ) && 
					 ( line_str[i + 1] != '0' ) )
				{
					word_str = ""; // init string for cuttenr word from string
					k = i + 1; // k = index of first symbol in current word
					do {
						word_str += line_str[k];
						k++;
						if ( k == line_str.length( ) ) { // skip empty or uncorrect line
							/*std :: cout << "\n***In ReadVarCount skipped line " << 
								           line_str << std::endl;*/
							IsUncorrectLine = true;
							break;
						}
					} while ( line_str[k] != ' ' );

					if ( IsUncorrectLine )
						break;

					current_lit_count++;
				}
			} // for ( i = 0; i < line_str.length( ) - 1; i++ )

			if ( current_lit_count ) {
				current_clause_count++;
				clause_lengths[current_clause_count - 1] = current_lit_count;
			}
		}
	}

	main_cnf.close( );
	
	return true;
}


//---------------------------------------------------------
bool MPI_Base :: ReadIntCNF()
{
// Reading CNF from file.
// Vars = {1, -1, 2, -2, ...} Literels = {2, 3, 4, 5, ...}
// clause_array        - array of clauses of literals
// lits_clause_array   - array of literals' clause indexes
// clause_num          - number of clauses
// var_count           - number of vars
// lits_num            - number of literals
// clause_lengths      - array of count of literals in clauses
// lits_clause_lengths - array of lengths of lits_clauses
    unsigned int current_clause_count = 0,
				 current_lit_count    = 0,
				 line_str_len         = 0,
				 current_var_count    = 0,
				 k                    = 0, 
				 val				  = 0,
				 sign				  = 0;
	unsigned first_obj_var;
	int lit_val;
	std::string line_str, 
		   word_str;
	bool IncorrectLine;
	std::vector<int> :: iterator vec_it;
    
	std::cout << "Start of ReadIntCNF()" << std::endl;
	if ( !ReadVarCount( ) ) {
		std::cerr << "Error in ReadVarCount" << std::endl; return false;
	}
	
	std::cout << "ReadVarCount() done" << std::endl;

	clause_array.resize( clause_count );

	for ( unsigned i = 0; i < clause_array.size(); ++i )
		clause_array[i].resize( clause_lengths[i] );

	// check file with main CNF
	std::ifstream main_cnf( input_cnf_name.c_str(), std::ios::in );
    if ( !main_cnf.is_open() ) {
		std::cerr << "Error in opening of file with input CNF with name" 
			 << input_cnf_name << std::endl;
		exit(1);
	}
	
	first_obj_var = 0;
	current_clause_count = 0;

	std::stringstream sstream;
	std::string str1, str2, str3, str4, str5;
	unsigned ui;
	bool Is_InpVar = false, Is_ConstrLen = false, Is_ObjLen = false, Is_ObjVars = false;
	while ( getline( main_cnf, line_str ) ) {
		if ( line_str[0] == 'p' )
			continue;
		if ( line_str[0] == 'c' ) { // in comment string can exist count of input variables
			//parse string for ex. "c 1452 input variables" or "c input variables 1452"
			sstream.str(""); sstream.clear();
			sstream << line_str;
			sstream >> str1 >> str2;
			
			/*if ( str2 == "rslos" ) {
				while ( sstream >> intval )
					rslos_lengths.push_back( intval );
				cout << "rslos_count " << rslos_lengths.size() << std::endl;
				cout << "rslos lens: ";
				for ( unsigned i = 0; i < rslos_lengths.size(); i++ )
					cout << rslos_lengths[i] << " ";
				cout << std::endl;
				
				continue;
			}*/

			sstream >> str3 >> str4; // get and parse words in string
			
			if ( str2 == "known_bits" ) {
				std::istringstream( str3 ) >> known_bits;
				std::cout << "known_bits " << known_bits << std::endl;
				continue;
			}
			if ( ( str2 == "output" ) && ( str3 == "variables" ) ) {
				std::istringstream( str4 ) >> output_len;
				continue;
			}
			if ((str2 == "output") && (str3 == "vars")) {
				sstream >> str5;
				std::istringstream(str5) >> output_len;
				continue;
			}
			if ( !Is_InpVar ) {
				if ((str2 == "input") && (str3 == "variables"))
					std::istringstream(str4) >> ui;
				else if ((str2 == "input") && (str3 == "vars")) {
					sstream >> str5;
					std::istringstream(str5) >> ui;
				}
				std::cout << "input variables " << ui << std::endl;
				if (!core_len)
					core_len = ui; // if core_len didn't set manually, read from file
				if ((core_len > MAX_CORE_LEN) || (core_len <= 0)) {
					core_len = MAX_CORE_LEN;
					std::cout << "Warning. core_len > MAX_CORE_LEN or <= 0. Changed to MAX_CORE_LEN" << std::endl;
					std::cout << "core_len " << core_len << " MAX_CORE_LEN " << MAX_CORE_LEN << std::endl;
				}
				Is_InpVar = true;
				continue;
			}
			sstream.str(""); sstream.clear();

			if ( str2 == "var_set" ) {
				sstream << line_str;
				std::cout << "line_str " << line_str << std::endl;
				sstream >> str1; // remove "c"
				sstream >> str2; // remove "var_set"
				while ( sstream >> val ) {
					std::cout << val << " ";
					full_var_choose_order.push_back( val );
				}
				std::cout << std::endl;
				sstream.clear(); sstream.str();
				sort( full_var_choose_order.begin(), full_var_choose_order.end() );
				std::cout << "After reading var_set" << std::endl;
				std::cout << "var_choose_order.size() " << full_var_choose_order.size() << std::endl;
				for ( unsigned i=0; i < full_var_choose_order.size(); ++i )
					std::cout << full_var_choose_order[i] << " ";
				std::cout << std::endl;
				core_len = full_var_choose_order.size();
				std::cout << "core_len changed to " << core_len << std::endl;
			}
		}
		else // if ( ( line_str[0] != 'p' ) && ( line_str[0] != 'c' ) )
		{
			// try to read line with clause
			current_lit_count = 0; // current count of lits in current clause
			line_str = " " + line_str;
			for ( unsigned i = 0; i < line_str.length( ) - 1; i++ ) {
				IncorrectLine = false;
				if ( ( line_str[i] == ' ' ) && ( line_str[i + 1] != ' ' ) && 
					 ( line_str[i + 1] != '0' ) )
				{
					word_str = "";
					k = i + 1;
					do {
						word_str += line_str[k];
						k++;
						if ( k == line_str.length( ) ) { // skip empty or uncorrect line
							/*std :: cout << "\n***In ReadVarCount skipped line " << 
								           line_str << std::endl;*/
							IncorrectLine = true;
							break;
						}
					} while ( line_str[k] != ' ' );

					if ( IncorrectLine )
						break;

					lit_val = atoi( word_str.c_str( ) ); // word -> lit value
					if ( !lit_val ) { // if non-number or '0' (lit > 0) then rerurn error;
						std::cout << "\n Error in ReadIntCNF. literal " << word_str << " is non-number";
						return false;
					}
					else if ( lit_val < 0 ) {
						lit_val = -lit_val; 
						sign = 1;
					}
					else if ( lit_val > 0 )
						sign = 0;

					// fill attay of clauses
					val = ( lit_val << 1 ) + sign; // literal value, 1 -> 2, -1 -> 3, 2 -> 4, -2 -> 5
					clause_array[current_clause_count][current_lit_count] = val;
					current_lit_count++;
				} // if ( ( line_str[i] == ' ' ) ...
			} // for ( i = 0; i < line_str.length( ) - 1; i++ )
			
			if ( ( te > 0 ) && ( current_lit_count == 1 ) )
				std::cout << "Warning. ( te > 0 ) && ( current_lit_count == 1 ). change CNF file to template one" << std::endl;

			if ( current_lit_count == 1 )
				known_vars_count++;
			
			if ( current_lit_count )
				current_clause_count++;
		} 
	} // while ( getline( main_cnf, line_str ) )
	
	// if PB data is correct then turn on PB mode
	/*if ( ( constr_clauses_count > 0 ) && ( obj_vars_count > 0 ) ) 
	{
		IsPB = true;
		solver_type = 3;
		if ( ( best_lower_bound > -1 ) && ( upper_bound > 0 ) ) // if bounds then equality mode
		{
			PB_mode = 2;
			if ( best_lower_bound > upper_bound )  // check correctness
				best_lower_bound = upper_bound;
		}
		else PB_mode = 1;
	}*/

	main_cnf.close( );
	
	// fill indexes of core variables
	/*k=0;
	for ( vec_it = full_var_choose_order.begin(); vec_it != full_var_choose_order.end(); ++vec_it )
		core_var_indexes.insert(std::pair<int,unsigned>( *vec_it, k++ ));
	if ( verbosity > 0 ) {
		std::cout << "core_var_indexes" << std::endl;
		for ( std::map<int,unsigned> :: iterator map_it = core_var_indexes.begin(); map_it != core_var_indexes.end(); ++map_it )
			std::cout << map_it->first << " " << map_it->second << std::endl;
	}*/

	nonoutput_len = var_count - output_len;
	
	// if wasn't defined by var_set
	if (full_var_choose_order.empty()) {
		if (known_bits) {
			core_len -= known_bits;
			for (unsigned i = 0; i < core_len; i++)
				full_var_choose_order.push_back(i + 1);
			std::cout << "new core_len (less due to nonzero value of known_bits) " << core_len << std::endl;
		}
		else {
			for (unsigned i = 0; i < nonoutput_len; i++)
				full_var_choose_order.push_back(i + 1);
		}
	}
	
	if ( ( isPredict ) && ( !core_len ) ) {
		std::cerr << "core_len == 0 in predict mode" << std::endl;
		return false;
	}
	
	std::cout << "ReadIntCNF() done" << std::endl;
	
	return true;
}

//---------------------------------------------------------
bool MPI_Base :: CheckSATset( std::vector<int> &lit_SAT_set_array )
{
// Check given SAT set
	int cnf_sat_val = 1,
		i = -1,
		checked_clauses = 0,
		j = -1,
		current_lit_val = -1,
		var_index = -1,
		current_sat_val = -1,
		clause_sat_val = -1;
	//
	i = 0;
	// check clauses while all of checked is SAT
	while ( ( cnf_sat_val ) && ( checked_clauses != clause_count ) ) {
		// add one to count of checked clauses in main CNF
        checked_clauses++;
		clause_sat_val = 0;
		j = 0;
		// check literals while all of checked is UNSAT
		while ( ( !clause_sat_val ) && ( j < clause_lengths[i] ) ) {
			current_lit_val = clause_array[i][j];
			var_index = current_lit_val/2;
			current_sat_val = lit_SAT_set_array[var_index - 1]; 
			if ( current_lit_val == current_sat_val )
				clause_sat_val++;
			j++;
		} // while ( ( !clause_sat_val ) && 
		//
		// if we found one UNSAT clause CNF is UNSAT too
		if ( !clause_sat_val )
			--cnf_sat_val;
		i++;
	} // while ( ( cnf_sat_val ) && ( checked_clauses != clause_num ) )
	return ( cnf_sat_val > 0 ) ? true : false;
}

//---------------------------------------------------------
bool MPI_Base :: AnalyzeSATset( double cnf_time_from_node )
{
// Reading of SAT set and check it for correction
	int lits_num = 0,
		int_answer = 0,
		sign = 0,
		val = 0,
		str_output_cur_ind = 0;
	unsigned answer_var_count;
	bool bIsSATSetExist = false;
	std::string answer_file_name,
		   output_file_name = "output",
		   str_answer,
		   line_buffer;
	std::ofstream answer_file,
			 output_file;
	std::vector<int> lit_SAT_set_array;
	std::stringstream sstream;
	
	std::cout << "Start of AnalyzeSATset" << std::endl;
	lit_SAT_set_array.resize( b_SAT_set_array.size() );
	for ( unsigned i = 0; i < lit_SAT_set_array.size(); ++i )
		lit_SAT_set_array[i] = ( ( i + 1 ) << 1 ) + (b_SAT_set_array[i] ? 0 : 1);
	// if SAT set exist then check it
	if ( verbosity > 0 )
		std::cout << "Before CheckSATset()" << std::endl;
	if ( !CheckSATset( lit_SAT_set_array ) ) {
		std::cerr << "Error in checking of SAT set" << std::endl;
		return false;
	}
	if ( verbosity > 0 )
		std::cout << "CheckSATset() done" << std::endl;
	
	// open file for writing of answer in zchaff style
	answer_file_name = "sat_sets_";
	answer_file_name += input_cnf_name;
	
	unsigned k = 1;
	for( unsigned i = 1; i < answer_file_name.length( ); ++i ) {
		if ( ( answer_file_name[i] == '.' ) || ( answer_file_name[i] == '\\' ) || 
			 ( answer_file_name[i] == '/' ) || ( answer_file_name[i] == ':' )  || 
			 ( ( answer_file_name[i] == '_' ) && ( answer_file_name[i - 1] == '_' ) ) )
			continue;
		else {
			answer_file_name[k] = answer_file_name[i];
			k++;
		}
	}
	answer_file_name.resize( k );
	sstream << "_" << rank;
	answer_file_name += sstream.str();
	sstream.clear(); sstream.str("");

	std::cout << "answer_file_name " << answer_file_name << std::endl; 
	answer_file.open( answer_file_name.c_str( ), std::ios::app );
	if ( !( answer_file.is_open( ) ) ) {
		std :: cerr << "Error in opening of file with answer in zchaff style " 
			        << answer_file_name << std::endl;
		return false;
	}
	
	answer_var_count = core_len;
	
#ifdef _MPI
	sstream << "total_time_from_start " << MPI_Wtime() - total_start_time << " s" << std::endl;
	sstream << cnf_time_from_node << " s SAT " << std::endl;
#endif
	for ( unsigned i = 0; i < b_SAT_set_array.size(); ++i )
		sstream << b_SAT_set_array[i];
	sstream << std::endl; 
	answer_file << sstream.rdbuf( );
	answer_file.close( );
	lit_SAT_set_array.clear();
	
	return true;
}

void MPI_Base :: MakeRandArr( std::vector< std::vector<unsigned> > &rand_arr, unsigned vec_len, unsigned rnd_uint32_count )
{
// make array of pseudorandom values using Mersenne Twister generator
	rand_arr.resize( vec_len );
	std::vector< std::vector<unsigned> > :: iterator it;
	for ( it = rand_arr.begin(); it != rand_arr.end(); it++ ) {
		(*it).resize( rnd_uint32_count );
		for ( unsigned j = 0; j < (*it).size(); j++ )
			(*it)[j] = uint_rand( gen );
	}
}

void MPI_Base :: MakeUniqueRandArr( std::vector<unsigned> &rand_arr, unsigned rand_arr_len, 
							        unsigned max_rand_val )
{
// make array of different pseudorandom values
	if ( max_rand_val < rand_arr_len )
		max_rand_val = rand_arr_len;
	unsigned rand_numb;
	rand_arr.resize( rand_arr_len );
	
	bool IsOldValue;
	for ( unsigned i = 0; i < rand_arr_len; i++ ) {
		do { // if value is not unique get value again
			rand_numb = uint_rand( gen );
			rand_numb %= max_rand_val;
			IsOldValue = false;
			for ( unsigned k = 0; k < i; ++k ) {
				if ( rand_numb == rand_arr[k] ) {
					IsOldValue = true;
					break;
				}
			}
		} while ( IsOldValue );
		rand_arr[i] = rand_numb; // new values
	}
}

void MPI_Base::MakeSingleSatSample(
		std::vector<bool> &state_vec, 
		std::vector<bool> &stream_vec, 
		const int seed_num,
		Solver *S,
		std::vector<lbool> predefined_vars)
{

	boost::random::mt19937 gen_local;
	//gen_local.seed( static_cast<unsigned>(seed_num^1234567));
	gen_local.seed( static_cast<unsigned>(seed_num));
	for (int i=0;i<100000;i++) gen_local;

	if (predefined_vars.empty())
		predefined_vars.resize(core_len, l_Undef);
	assert( predefined_vars.size()==core_len);

	vec<Lit> dummy;
	for ( unsigned i=0; i < core_len; i++ ){
		state_vec.push_back(l_Undef==predefined_vars[i]? bool_rand(gen_local) : l_True==predefined_vars[i]);
		//state_vec.push_back(bool_rand(gen_local));
		dummy.push(~mkLit(i, state_vec[i]));
	}
	Minisat::lbool ret = S->solveLimited( dummy );

	if ( ret != l_True ) {
		std::cerr << "in makeSatSample() ret != l_True" << std::endl;
		exit(1);
	}

	for( int i=state_vec.size(); i < S->model.size() - (int)output_len; i++ )
		state_vec.push_back( (S->model[i] == l_True) ? true : false );
	for( int i=S->model.size() - output_len; i < S->model.size(); i++ )
		stream_vec.push_back( (S->model[i] == l_True) ? true : false );
}

void MPI_Base::MakeSatSample(std::vector< std::vector<bool> > &state_vec_vec,
							 std::vector< std::vector<bool> > &stream_vec_vec,
							 std::vector< std::vector<bool> > &plain_text_vec_vec,
							 int rank)
{
	std::stringstream sstream;
	sstream << "known_sat_sample_" << rank;
	std::string known_sat_sample_file_name = sstream.str();
	sstream.str(""); sstream.clear();
	std::fstream known_sat_sample_file(known_sat_sample_file_name, std::ios_base::in );
	std::vector<bool> state_vec, stream_vec, plain_text_vec;
	
	if ( ( isMakeSatSampleAnyWay ) || (!known_sat_sample_file.is_open()) ) { // empty file
	//if ( file.peek() == fstream::traits_type::eof() ) { // if file is empty
		// make [sample_size] different pairs <register_state, keystream> via generating secret keys
		std::cout << "file known_sat_sample is empty. making SAT sample" << std::endl;
		
		boost::random::mt19937 gen_known_vars;
		const unsigned known_vars_seed = static_cast <unsigned> (std::time(NULL)) + (unsigned)rank * 1000000;
		std::cout << "known_vars_seed " << known_vars_seed << std::endl;
		gen_known_vars.seed(known_vars_seed);
		
		// additionally plaintext is nedded 
		/*if (isPlainText) {
			unsigned ciphertext_len = 0;
			if (input_cnf_name.find("32") != std::string::npos)
				ciphertext_len = 32;
			else if (input_cnf_name.find("64") != std::string::npos)
				ciphertext_len = 64;
			else if (input_cnf_name.find("128") != std::string::npos)
				ciphertext_len = 128;
			else {
				std::cerr << "ciphertext_len == 0" << std::endl;
				exit(1);
			}
			std::cout << "des_ciphertext_len " << ciphertext_len << std::endl;
			plain_text_vec.resize(ciphertext_len);
			for (unsigned i = 0; i < cnf_in_set_count; i++) {
				for (unsigned j = 0; j < ciphertext_len; j++)
					plain_text_vec[j] = bool_rand(gen);
				plain_text_vec_vec.push_back(plain_text_vec);
			}
			
			std::cout << "plain_text_vec_vec.size() " << plain_text_vec_vec.size() << std::endl;
			std::cout << "plain_text_vec_vec[0].size() " << plain_text_vec_vec[0].size() << std::endl;
			
			for (unsigned i = 0; i < plain_text_vec_vec.size(); i++)
				for (unsigned j = 0; j < plain_text_vec_vec[i].size(); j++)
					state_vec_vec[i].push_back(plain_text_vec_vec[i][j]);

			std::cout << "state_vec_vec.size() " << state_vec_vec.size() << std::endl;
			std::cout << "state_vec_vec[0].size() " << state_vec_vec[0].size() << std::endl;
		}*/
		
		// get state of additional variables
		Problem cnf;
		Solver *S;
		Minisat::lbool ret;
		minisat22_wrapper m22_wrapper;
		std::ifstream in( input_cnf_name.c_str() );
		m22_wrapper.parse_DIMACS_to_problem(in, cnf);
		in.close();
		S = new Solver();
		S->addProblem(cnf);
		vec<Lit> dummy;
		int cur_var_ind;

		/*state_vec.resize(core_len);
		for (unsigned i = 0; i < cnf_in_set_count; i++) {
		for (unsigned j = 0; j < core_len; j++)
		state_vec[j] = bool_rand(gen_known_vars);
		state_vec_vec.push_back(state_vec);
		}*/

		//int state_vec_len = state_vec_vec[0].size();
		//for ( std::vector< std::vector<bool> > :: iterator x = state_vec_vec.begin(); x != state_vec_vec.end(); x++ ) 
		unsigned long long unsat_genereted_count = 0, undef_genereted_count = 0;
		do
		{
			state_vec.resize(core_len);
			for (unsigned i = 0; i < core_len; i++)
				state_vec[i] = bool_rand(gen_known_vars);
			cur_var_ind = 0;
			for (unsigned i = 0; i < state_vec.size(); i++) {
				dummy.push( state_vec[i] ? mkLit( cur_var_ind ) : ~mkLit( cur_var_ind ) );
				cur_var_ind++;
			}
			ret = S->solveLimited( dummy );
			dummy.clear();
			if ( ret == l_True ) {
				for (int i = core_len; i < S->model.size() - (int)output_len; i++)
					state_vec.push_back((S->model[i] == l_True) ? true : false);
				state_vec_vec.push_back(state_vec);
				for (int i = S->model.size() - output_len; i < S->model.size(); i++)
					stream_vec.push_back((S->model[i] == l_True) ? true : false);
				stream_vec_vec.push_back(stream_vec);
				stream_vec.clear();
			}
			else if (ret == l_False)
				unsat_genereted_count++;
			else if (ret == l_Undef)
				undef_genereted_count++;
			if ((unsat_genereted_count) && (unsat_genereted_count % 100000 == 0)) {
				std::cout << "unsat_generated_count " << unsat_genereted_count << std::endl;
				//std::cout << "undef_generated_count " << undef_genereted_count << std::endl;
				std::cout << "state_vec_vec.size() " << state_vec_vec.size() << std::endl;
			}
		} while (state_vec_vec.size() < cnf_in_set_count);
		
		std::cout << "unsat_generated_count " << unsat_genereted_count << std::endl;
		sstream << "state" << std::endl;
		for ( std::vector< std::vector<bool> > :: iterator x = state_vec_vec.begin(); x != state_vec_vec.end(); x++ ) {
			for ( std::vector<bool> :: iterator y = (*x).begin(); y != (*x).end(); y++ )
				sstream << *y;
			sstream << std::endl;
		}
		sstream << "stream" << std::endl;
		for ( std::vector< std::vector<bool> >::iterator x = stream_vec_vec.begin(); x != stream_vec_vec.end(); x++ ) {
			for ( std::vector<bool>::iterator y = (*x).begin(); y != (*x).end(); y++ )
				sstream << *y;
			sstream << std::endl;
		}
		known_sat_sample_file.close(); known_sat_sample_file.clear();
		known_sat_sample_file.open(known_sat_sample_file_name, std::ios_base::out);
		known_sat_sample_file << sstream.rdbuf();
		delete S;
	}
	else {
		std::string str;
		getline(known_sat_sample_file,str);
		std::cout << "reading state and stream from file" << std::endl;
		bool isState = false, isStream = false;
		do {
			if( str == "state" ) {
				std::cout << "state string found" << std::endl;
				isState = true;
			}
			else if ( str == "stream" ) {
				std::cout << "stream string found" << std::endl;
				isState = false;
				isStream = true;
			}
			else {
				if ( isState ) {
					for ( unsigned i=0; i < str.size(); i++ )
						state_vec.push_back( str[i] == '1' ? true : false );
					state_vec_vec.push_back( state_vec );
					state_vec.clear();
				}
				else if ( isStream ) {
					for ( unsigned i=0; i < str.size(); i++ )
						stream_vec.push_back( str[i] == '1' ? true : false );
					stream_vec_vec.push_back( stream_vec );
					stream_vec.clear();
				}
			}
		} while( getline(known_sat_sample_file, str ) );
		std::cout << "state_vec_vec.size() "  << state_vec_vec.size()  << std::endl;
		std::cout << "stream_vec_vec.size() " << stream_vec_vec.size() << std::endl;
	}
	std::cout << std::endl;
	known_sat_sample_file.close();
	
	/*if (isPlainText) // return size of input vectors without plain text data
		for (auto &x : state_vec_vec)
			x.resize(core_len);*/
}

std::string MPI_Base::MakeSolverLaunchString( std::string solver_name, std::string cnf_name, double maxtime_solving_time )
{
	std::string time_limit_str, result_str;
	std::stringstream sstream;
	sstream << max_solving_time;
	std::string maxtime_seconds_str = sstream.str();
	sstream.clear(); sstream.str("");
	// minisat and solvers with same time limit paremeter
	if ( solver_name.find( "cryptominisat" ) != std::string::npos )
		time_limit_str = "";
	else if ( ( solver_name.find( "minisat" ) != std::string::npos ) || 
		 ( solver_name.find( "Minisat" ) != std::string::npos ) || 
		 ( solver_name.find( "MiniSat" ) != std::string::npos ) ||
		 ( solver_name.find( "minigolf" ) != std::string::npos ) ||
		 ( solver_name.find( "mipisat" ) != std::string::npos ) ||
		 ( solver_name.find( "minitsat" ) != std::string::npos ) ||
		 ( solver_name.find( "rokk" ) != std::string::npos ) ||
		 ( solver_name.find( "sinn" ) != std::string::npos ) ||
		 ( solver_name.find( "zenn" ) != std::string::npos ) ||
		 ( solver_name.find( "SWDiA5BY" ) != std::string::npos ) ||
		 ( solver_name.find( "ClauseSplit" ) != std::string::npos ) 
		 )
	{
		time_limit_str =  "-cpu-lim=";
	}
	else if ( ( solver_name.find( "lingeling" ) != std::string::npos ) && 
		      ( solver_name.find( "plingeling" ) == std::string::npos ) ) {
		time_limit_str = "-t ";
	}
	else {
		time_limit_str = "";
		//std::cerr << "Unknown solver in system calling mode: " << solver_name << std::endl;
		//MPI_Abort( MPI_COMM_WORLD, 0 );
	}
	// glucose can't stop in time
	/*if ( solver_name.find( "glucose" ) != std::string::npos ) {
		std::cout << "glucose detected" << std::endl;
		result_str = "-cpu-lim=";
	}*/
	/*else if ( solver_name.find( "plingeling" ) != std::string::npos ) {
		//std::cout << "pingeling detected" << std::endl;
		result_str = "-nof_threads ";
		result_str += nof_threads_str;
		result_str += " -t ";
	}
	else if ( solver_name.find( "trengeling" ) != std::string::npos ) {
		//std::cout << "treengeling detected" << std::endl;
		//result_str = "-t " + "11" + nof_threads_str;

	if ( solver_name.find( "dimetheus" ) != std::string::npos )
		result_str += " -formula";
	}
	
	if ( time_limit_str == "" ) {
		std::cout << "unknown solver detected. using timelimit" << std::endl;
		result_str = "./timelimit -t " + maxtime_seconds_str + " -T 1 " + "./" + solvers_dir + "/" + solver_name;
	}*/
	
	if ( time_limit_str != "" )
		result_str = solver_name + " " + time_limit_str + maxtime_seconds_str + " " + cnf_name;
	else
		result_str = solver_name + " " + cnf_name;
	
	return result_str;
}
