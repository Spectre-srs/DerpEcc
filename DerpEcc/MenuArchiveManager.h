// Ecc Archive Management Menu - modify or view archive contents
// takes ecfArchive, and produces according realFiles
// goes to MenuFileWithEcc with realFile and ecFile

//todo: select file by mask WITH IGNORE CASE PLEASE
//todo: select file by mass, <, >, ==
//todo: add and save are treating localpath and globalpath to the same file as different files, it shouldn't
/*
   setlocale(LC_ALL,"C");   // in effect by default  
   printf("\n%d",_wcsicmp(L"ä", L"Ä"));   // compare fails  
   setlocale(LC_ALL,"");  
   printf("\n%d",_wcsicmp(L"ä", L"Ä"));   // compare succeeds  
*/
/*
use the GetFullPathName from kernel32.dll
GetFullPathName(TEXT("C:\\Temp\\..\\autoexec.bat"),1000,buffer1,NULL);
GetFullPathName(TEXT("C:\\autoexec.bat"),1000,buffer2,NULL);
GetFullPathName(TEXT("\\autoexec.bat"),1000,buffer3,NULL);
GetFullPathName(TEXT("C:/autoexec.bat"),1000,buffer4,NULL);
*/
#include <map>



//-----------------------------------------------------------------------------
class rsEcfArchiveManager
{
public:
	typedef std::vector<rsFilePair>::iterator iterator;

	rsEcfArchiveManager( rsEcfArchive& ecfArchive )
		: dirty( ecfArchive.dirty )
		, ecFiles( ecfArchive.ecFiles )
	{
		const std::size_t amount = ecFiles.size();
		filePairs.resize( amount, rsFilePair(nullptr, nullptr, realFiles, ecFiles) );
		for( std::size_t i = 0; i < amount; i++ )
		{
			filePairs[i].ecFile = ecFiles[i].get();
			filePairs[i].Update();
		}
	}

	void StoreEcFile( rsFilePair& pair )
	{
		ENFORCE( !pair.ecFile )("backup already exists");
		NOT_EMPTY_PAIR( pair );

		uniq_ptr<rsFile> ecFile = new rsFile();
		*ecFile = *pair.realFile;
		pair.ecFile = ecFile.get();
		pair.Update();
		ecFiles.push_back( ecFile );
		dirty = true;
	}

	iterator AddEcFile( uniq_ptr<rsFile>& ecFile )
	{
		ASSERT( ecFile )("null pointer");
		filePairs.push_back( rsFilePair(nullptr, ecFile.get(), realFiles, ecFiles) );
		ecFiles.push_back( ecFile );
		dirty = true;
		return --filePairs.end();
	}

	void CaptureRealFile( rsFilePair& pair )
	{
		ENFORCE( !pair.realFile )("file already found");
		NOT_EMPTY_PAIR( pair );

		uniq_ptr<rsFile> realFile = ReadFileData( pair.ecFile->name );
		pair.realFile = realFile.get();
		pair.Update();
		realFiles.push_back( realFile );
	}

	iterator AddRealFile( uniq_ptr<rsFile>& realFile )
	{
		ASSERT( realFile )("null pointer");
		filePairs.push_back( rsFilePair(realFile.get(), nullptr, realFiles, ecFiles) );
		realFiles.push_back( realFile );
		return --filePairs.end();
	}

	iterator AddRealIfMatches( uniq_ptr<rsFile>& realFile )
	{
		iterator it = FindCompatibleEcFile( realFile.get() );
		if( it != filePairs.end() )
		{
			ASSERT( !it->realFile && it->ecFile )("compatible ecFile is incompatible");
			it->realFile = realFile.get();
			it->Update();
			realFiles.push_back( realFile );
		}
		return it;
	}

	iterator Find( const std::wstring& fileName )
	{
		struct {
			const std::wstring& name;
			bool operator () ( const rsFilePair& pair ) {
				//!_wcsicmp(f.realFile->name.c_str(), name.c_str())
				if( pair.realFile )
					if( pair.realFile->name == name ) return true;
				if( pair.ecFile )
					if( pair.ecFile->name == name ) return true;
				NOT_EMPTY_PAIR( pair );
				return false;
			}
		} cmp = { fileName };

		return std::find_if( filePairs.begin(), filePairs.end(), cmp );
	}

	template<typename pred_t>
	void Search( pred_t& filter, std::vector<rsFilePair*>& results )
	{
		for( std::size_t i = 0, e = filePairs.size(); i < e; i++ )
			if( filter(filePairs[i]) ) results.push_back( &filePairs[i] );
	}

	void Remove( iterator it )
	{
		ENFORCE( filePairs.size() > 0 )("nothing is available to remove");
		ENFORCE( filePairs.end() != it )("trying to remove end iterator");

		if( it->realFile ) DropRealFile( *it );
		if( it->ecFile ) DropEcFile( *it );
		filePairs.erase( it );
	}

	void Remove( std::vector<rsFilePair*>& selection )
	{
		ENFORCE( filePairs.size() > 0 )("nothing is available to remove");
		ENFORCE( selection.size() > 0 )("nothing is selected to remove");

		std::sort( selection.rbegin(), selection.rend() );
		for( std::size_t i = 0, e = selection.size(); i < e; i++ )
			Remove( Pointer2Iterator(selection[i]) );
	}

	void CaptureRealFiles()
	{
		realFiles.reserve( filePairs.size() );
		for( std::size_t i = 0, e = filePairs.size(); i < e; i++ )
		{
			try { CaptureRealFile( filePairs[i] ); }
			catch ( std::runtime_error& ) {}
		}
	}

	void CaptureRealFiles( std::vector<rsFilePair*>& selection )
	{
		realFiles.reserve( std::min(filePairs.size(), selection.size()) );
		for( std::size_t i = 0, e = selection.size(); i < e; i++ )
		{
			try { CaptureRealFile( *selection[i] ); }
			catch ( std::runtime_error& ) {}
		}
	}

	void MatchRealFilesToEc()
	{
		for( std::size_t i = 0, e = filePairs.size(); i < e; i++ )
		{
			if( filePairs[i].ecFile ) continue;
			NOT_EMPTY_PAIR( filePairs[i] );

			iterator ecPair = FindCompatibleEcFile( filePairs[i].realFile );
			if( ecPair == end() ) continue;

			Merge( filePairs[i], *ecPair );
			i--;
			e--;
		}
	}

	void DropRealFiles( std::vector<rsFilePair*>& selection )
	{
		ENFORCE( filePairs.size() > 0 )("nothing is available to release");
		ENFORCE( selection.size() > 0 )("nothing is selected to release");

		std::sort( selection.rbegin(), selection.rend() );
		for( std::size_t i = 0, e = selection.size(); i < e; i++ )
		{
			if( !selection[i]->realFile ) continue;

			DropRealFile( *selection[i] );
			if( !selection[i]->ecFile )
				filePairs.erase( Pointer2Iterator(selection[i]) );
		}
		selection.clear();
	}

	void Sort()
	{
		dirty = true;

		struct {
			bool operator () ( const uniq_ptr<rsFile>& left, const uniq_ptr<rsFile>& right )
			{
				const std::wstring delim = L"\\/";
				const std::vector<std::wstring> leftPath = split( left->name, delim );
				const std::vector<std::wstring> rightPath = split( right->name, delim );

				for( std::size_t i = 0, e = std::min(leftPath.size(), rightPath.size()); i < e; ++i )
				{
					int comparison = _wcsicmp( leftPath[i].c_str(), rightPath[i].c_str() );
					if( !comparison ) continue;

					const bool leftIsDir = i < leftPath.size() - 1;
					const bool rightIsDir = i < rightPath.size() - 1;

					if( leftIsDir == rightIsDir )
						return comparison < 0;
					else
						return rightIsDir;
				}

				ASSERT( false )("sorting function detected a file duplicate: '")(left->name)("'");
				return false;
			}
		} byPathName;
		std::sort( ecFiles.begin(), ecFiles.end(), byPathName );

		struct {
			const std::vector<uniq_ptr<rsFile>>& ecFiles;
			const std::vector<uniq_ptr<rsFile>>& realFiles;
			bool operator () ( const rsFilePair& left, const rsFilePair& right )
			{
				NOT_EMPTY_PAIR( left );
				NOT_EMPTY_PAIR( right );
				if( left.ecFile && right.ecFile )
				{
					for( std::size_t i = 0, e = ecFiles.size(); i < e; ++i )
					{
						if( ecFiles[i] == right.ecFile ) return false;
						if( ecFiles[i] == left.ecFile ) return true;
					}
					ASSERT( false )("sorting function detected pair with invalid pointers: '")
						(left.ecFile->name)("' and '")(right.ecFile->name)("'");
				}
				else if( !left.ecFile && !right.ecFile )
				{
					for( std::size_t i = 0, e = realFiles.size(); i < e; ++i )
					{
						if( realFiles[i] == right.realFile ) return false;
						if( realFiles[i] == left.realFile ) return true;
					}
					ASSERT( false )("sorting function detected pair with invalid pointers: '")
						(left.realFile->name)("' and '")(right.realFile->name)("'");
				}
				return !right.ecFile;
			}
		} bySortedFiles = { ecFiles, realFiles };
		std::sort( filePairs.begin(), filePairs.end(), bySortedFiles );
	}

	iterator begin()
	{
		return filePairs.begin();
	}

	iterator end()
	{
		return filePairs.end();
	}

	std::size_t size() const
	{
		return filePairs.size();
	}

	void reserve( std::size_t size )
	{
		filePairs.reserve( size );
		ecFiles.reserve( size );
		realFiles.reserve( size );
	}

	rsFilePair& operator [] ( std::size_t pos )
	{
		return filePairs[pos];
	}

	const rsFilePair& operator [] ( std::size_t pos ) const
	{
		return filePairs[pos];
	}

private:
	std::vector<uniq_ptr<rsFile>>& ecFiles;
	std::vector<uniq_ptr<rsFile>> realFiles;
	std::vector<rsFilePair> filePairs;
	bool& dirty;

	void DropRealFile( rsFilePair& pair )
	{
		ASSERT( pair.realFile )("null pointer");
		std::vector<uniq_ptr<rsFile>>::iterator index =
			std::find( realFiles.begin(), realFiles.end(), pair.realFile );
		ASSERT( index != realFiles.end() )("invalid realFile in filePairs!");
		realFiles.erase( index );
		pair.realFile = nullptr;
		pair.Update();
	}

	void DropEcFile( rsFilePair& pair )
	{
		ASSERT( pair.ecFile )("null pointer");
		std::vector<uniq_ptr<rsFile>>::iterator index =
			std::find( ecFiles.begin(), ecFiles.end(), pair.ecFile );
		ASSERT( index != ecFiles.end() )("invalid ecFile in filePairs!");
		ecFiles.erase( index );
		pair.ecFile = nullptr;
		pair.Update();
		dirty = true;
	}

	iterator FindCompatibleEcFile( const rsFile* realFile )
	{
		ASSERT( realFile )("should not be null");

		rsFileComparison d;
		for( std::size_t i = 0, e = filePairs.size(); i < e; i++ )
		{
			if( filePairs[i].realFile ) continue;
			NOT_EMPTY_PAIR( filePairs[i] );

			d.Compare( realFile, filePairs[i].ecFile );
			bool metamatch = d.EqualMeta() || d.IsCopied() || d.IsMoved() || d.IsOverwriten();
			bool binmatch = d.EqualSize() && d.Md5Equal() && !d.EccDiffers();
			if( metamatch || binmatch ) return filePairs.begin() + i;
		}
		return filePairs.end();
	}

	void Merge( rsFilePair& realFilePair, rsFilePair& ecFilePair )
	{
		ASSERT( realFilePair.realFile && !realFilePair.ecFile )("invalid merge - pair already occupied");
		ASSERT( !ecFilePair.realFile && ecFilePair.ecFile )("invalid merge - pair already occupied");
		ecFilePair.realFile = realFilePair.realFile;
		ecFilePair.Update();
		realFilePair.realFile = nullptr;
		filePairs.erase( Pointer2Iterator(&realFilePair) );
	}

	inline iterator Pointer2Iterator( const rsFilePair* pair )
	{
		const rsFilePair* begin = &filePairs[0];
		ASSERT( pair >= begin )("invalid pair pointer leading outside of the array");
		const std::size_t index = pair - begin; //pointer magic arithmetic
		ASSERT( index < filePairs.size() )("invalid pair pointer leading outside of the array");
		return filePairs.begin() + index;
	}
/*
	void Link( rsFilePair* left, rsFilePair* right )
	{
		bool ecIsLeft = !left->realFile;
		if( ecIsLeft )
			Merge( right, left );
		else
			Merge( left, right );
	}*/
};



//-----------------------------------------------------------------------------
static bool NoEcc( const rsFilePair& pair )
{
	if( !pair.ecFile ) return false;
	return pair.ecFile->data.Size() == 0;
}
static bool Ecc0( const rsFilePair& pair )
{
	if( !pair.ecFile ) return false;
	return pair.ecFile->data.dataType == rsData::DT_32kbRAW_1mbECC;
}
static bool Ecc1( const rsFilePair& pair )
{
	if( !pair.ecFile ) return false;
	return pair.ecFile->data.dataType == rsData::DT_12kbECC_1mbDATA;
}
static bool All( const rsFilePair& pair )
{
	return true;
}
struct Categorized
{
	const FILE_COMPARISON_CATEGORY target;
	Categorized( FILE_COMPARISON_CATEGORY cat ) : target( cat ) {}
	bool operator () ( const rsFilePair& pair ) {
		return pair.diff == target;
	}
};
struct Mask
{
	std::vector<std::wstring> Keys;
	Mask( const std::wstring& mask ) {
		Keys = split( mask, std::wstring(L"*") );
		if( *mask.begin() == '*' ) Keys.insert( Keys.begin(), L"*" );
		if( *mask.rbegin() == '*' ) Keys.push_back( L"*" );
	}
	bool operator () ( const rsFilePair& pair ) {
		NOT_EMPTY_PAIR( pair );
		if( Keys.empty() ) return false;
		std::size_t i = 0;
		std::size_t e = Keys.size();
		std::size_t pos = 0;
		std::size_t tpos = 0;
		bool use_tpos = false;
		rsFile* cmpFile = (pair.ecFile) ? pair.ecFile : pair.realFile;

		if( Keys.front() != L"*" )
		{
			//if( _wcsicmp( pair.ecFile->name.substr(0, Keys.front().size()).c_str(), Keys.front().c_str() ) )
			if( cmpFile->name.compare(0, Keys.front().size(), Keys.front()) )
				return false;
		}
		else ++i;

		if( Keys.back() != L"*" )
		{
			tpos = cmpFile->name.size() - Keys.back().size();
			use_tpos = true;
		}
		else --e;

		for( ; i < e; i++ )
		{
			pos = cmpFile->name.find( Keys[i], pos );
			if( pos == std::wstring::npos ) return false;
		}
		if( use_tpos && tpos != pos ) return false;
		return true;
	}
};
struct ExistsInSortedSelection
{
	std::vector<rsFilePair*>& list;
	ExistsInSortedSelection( std::vector<rsFilePair*>& selection ) : list( selection ) {}
	bool operator () ( const rsFilePair* pair ) const {
		return std::binary_search( list.begin(), list.end(), pair );
	}
	typedef rsFilePair* argument_type;
};



//-----------------------------------------------------------------------------
class MenuArchiveManager
{

	/*void fix_( const std::wstring& )
	{
		ENFORCE( realFile->data.Size() > 0 )
			("you haven't checked if the file is already healthy! use forcefix instead");

		ENFORCE( realFile->data.Size() == ecFile->data.Size() )
			("ecc length mismatch for file '")(realFile->name)("'");

		forcefix();
	};

	void forcefix()
	{
		const std::wstring fixedFileName = realFile->name + L".fix";
		RestoreFromEcc( *realFile, *ecFile, fixedFileName );

GetFileType( const std::wstring& realFileName ) != FT_INVALID
		uniq_ptr<rsFile> fixedFile = ReadFileData( fixedFileName );
		fixedFile->createDate = ecFile->createDate;
		fixedFile->writeDate = ecFile->writeDate;
		RestoreFileDate( *fixedFile );

		std::cout << "finished: " << fixedFileName << std::endl;
	}*/

	void fix( const std::wstring& command )
	{
		EnforceSelectionNotEmpty();

		FILE_TYPE fileType = GetFileType( command );
		ENFORCE( fileType != FT_FILE )("file '")(command)("' already exists, overwriting is not an option");
		//todo: make it an option

		std::vector<std::wstring> filenames;

		if( fileType == FT_INVALID )
		{
			ENFORCE( selection.size() == 1 )("select only one file to save fixed version under that name");

			filenames.push_back( command );
		}
		else if( fileType == FT_DIR )
		{
			filenames.resize( selection.size() );
			for( std::size_t i = 0, e = selection.size(); i < e; i++ )
			{
				if( selection[i]->ecFile && selection[i]->ecFile->data.Size() )
				{
					const std::wstring& filename = selection[i]->ecFile->name;
					size_t pos = 0;
					{
						size_t pos1 = filename.rfind(L"/");
						size_t pos2 = filename.rfind(L"\\");
						if( pos1 != std::wstring::npos ) pos = pos1;
						if( pos2 != std::wstring::npos ) pos = std::max( pos1, pos2 );
					}
					filenames[i] = command + L"\\" + filename.substr(pos);
				}
			}
		}

		CounterSetTarget( selection.size(), GetSelectionMass(),
			"  - writing error corrected output" );
		for( std::size_t i = 0, e = selection.size(); i < e; i++ )
		{
			try
			{
				ENFORCE( selection[i]->ecFile != nullptr )("no backup for this file");
				ENFORCE( selection[i]->ecFile->data.Size() != 0 )("no error correction data for this file");
				ENFORCE( selection[i]->realFile != nullptr )("no source file to read from for this file");
				RestoreFromEcc( *(selection[i]->realFile), *(selection[i]->ecFile), filenames[i] );
			} catch ( std::runtime_error& e ) {
				std::cout << e.what() << std::endl;
				if( selection[i]->ecFile )
					CounterAdvanceMass( selection[i]->ecFile->fileSize );
			}
			CounterAdvanceCount( 1 );
		}
	}

	void link( const std::wstring& fileName )
	{
		rsEcfArchiveManager::iterator file = fileList.Find( fileName );
		ENFORCE( file != fileList.end() )("file '")(fileName)("' not found");

		//MenuFileWithEcc( file->realFile, file->ecFile );
		ecfArchive.dirty |= file->ecFile->dirty;
	}
public:
	bool autoHelp;
	MenuArchiveManager( rsEcfArchive& ecfArchive )
		: ecfArchive( ecfArchive )
		, fileList( ecfArchive )
		, autoHelp( false )
	{
		{
			std::wstring status = ecfArchive.fsName + L"  - scanning for files";
			SetTitle( status.c_str() );
		}

		WorkingDirectory chDir( FilePathFromPath(ecfArchive.fsName) );
		if( fileList.size() > 500 )
			std::cout << "more than 500 files in archive, skipping autocapture" << std::endl;
		else
			fileList.CaptureRealFiles();
		help();
		go();
	}

	void help( const std::wstring& param = L"", bool faggot = false )
	{
		const bool full = !param.empty();
		const std::size_t files = fileList.size();
		const std::size_t selected = selection.size();

		rsFileCategories fileStat;
		if( selection.empty() || full ) {
			fileStat.Count( fileList );
		} else {
			std::cout << "\nin this selection:" << std::endl;
			fileStat.Count( selection );
		}
		print( fileStat );
		if( files ) std::cout << std::endl;

		if( !faggot && !autoHelp ) return;

		if( !files || full ) std::cout <<
			"help - show relevant command list, call with any argument to show all commands" << std::endl;
		if( !files || full ) std::cout <<
			"autohelp - toggle automatic display of contextual commands after most actions" << std::endl;
		if( full ) std::cout <<
			"dirty - mark archive as dirty without making changes" << std::endl;
		if( full ) std::cout <<
			"font - set output codepage to russian" << std::endl;
		if( fileStat[orphan] || full ) std::cout <<
			"capture - try to search for lost files by backup name again, selected or all" << std::endl; //doesn't write amount of captured files
		if( fileStat[orphan] || full ) std::cout <<
			"match - try to search for lost files by comparing md5 with added" << std::endl; //import function from manager
		if( (files && !selected) || full ) std::cout <<
			"release - drop selected captured files for recapture and recalculation" << std::endl;
		if( !selected || full ) std::cout <<
			"add - open a file or folder to add" << ((fileStat[orphan]>0)?" or point out lost files":"") << std::endl;
		if( !selected || full ) std::cout <<
			"addr - same as add, but including all subfolders too" << std::endl;
		if( selected || full ) std::cout <<
			"remove - delete selected files' backups and remove them from list" << std::endl;
		if( (files && !selected) || full ) std::cout <<
			"select - choose a new group of files by mask to work with, call empty for help" << std::endl;
		if( selected || full ) std::cout <<
			"deselect - cancel selection" << std::endl;
		if( selected || full ) std::cout <<
			"inverse - select currently unselected files" << std::endl;
		if( selected || full ) std::cout <<
			"include - choose a group of files by mask to add to selection" << std::endl;
		if( selected || full ) std::cout <<
			"narrow - choose a group of files by mask from current selection" << std::endl;
		if( selected || full ) std::cout <<
			"exclude - choose a group of files by mask to remove from selection" << std::endl;
		if( selected || full ) std::cout <<
			"list - output names of currently selected files: " << selected << std::endl;
		if( selected || full ) std::cout <<
			"info - output full info about selected files: " << selected << std::endl;
		if( selected || full ) std::cout <<
			"folders - list directory and sizes of selected files" << std::endl; //count realFiles or ecFiles?
		if( (files && !selected) || full ) std::cout <<
			"sort - rearrange archive contents by name" << std::endl;
		if( selected || full ) std::cout <<
			"ecc - calculate ecc and md5 for selected files" << std::endl; //has unstated parameter //add "md5 changed!" warning
		if( selected || full ) std::cout <<
			"md5 - calculate only md5 for selected files" << std::endl; //has unstated parameter //add "md5 changed!" warning
		if( selected || full ) std::cout <<
			"update - write compatible changes/additions to the backup" << std::endl;
		if( (selected && fileStat[copied]) || full ) std::cout <<
			"uncopy - restore original creation data for selected files" << std::endl; //has unstated hack parameter
		if( (selected && fileStat[moved]) || full ) std::cout <<
			"unmove - restore original name and location for selected files" << std::endl; //not implemented dura
		if( selected || full ) std::cout <<
			"overwrite - delete backup content and replace it with current state of the file" << std::endl;
		if( ecfArchive.Dirty() || full ) std::cout <<
			"save - apply changes to the archive, or save as new under supplied name" << std::endl; //add fucking targetname
		if( (files && !selected) || full ) std::cout <<
			"import - add ec files from another ecf archive" << std::endl;
		if( selected || full ) std::cout <<
			"export - add selected ec files to another ecf archive" << std::endl;
		if( !selected || full ) std::cout <<
			"close - " << ((ecfArchive.Dirty())?"save and ":"") << "quit archive" << std::endl;
	}

	void Header()
	{
		//somewhy if file is 1 char length, set title fails
		//todo: make it archive estimated mass on disk instead
		std::wstring status = ecfArchive.fsName + L" ";
		//"E:\Documents and Settings\Administrator\My Documents\ICQ Lite\346243013\..."
		//max header length, cut it to "E:\Docu...\346243013\derpecc.ecf" when it gets too long
		const size_t splitLen = 50;
		const size_t leftSizeLen = 8;

		if( ecfArchive.fsName.length() > 50 )
		{
			status = ecfArchive.fsName.substr( 0, leftSizeLen );
			status += L"***";
			size_t splitPos = ecfArchive.fsName.rfind( L"\\" );
			if( (splitPos - 1) < splitPos )
			{
				size_t folderPos = ecfArchive.fsName.rfind( L"\\", splitPos - 1 );
				if( (ecfArchive.fsName.length() - folderPos + leftSizeLen + 3) < splitLen )
				{
					splitPos = folderPos;
				}
			}
			status += ecfArchive.fsName.substr( splitPos );
			status += L" ";
		}
		if( selection.size() )
		{
			std::string append = " - files selected: ";
			char num[33];
			_itoa_s( size_t_u32(selection.size()), num, sizeof(num), 10 );
			append += num;
			append += ", ";
			append += FileSizeToString( GetSelectionMass() );
			status += std::wstring( append.begin(), append.end() );
		}
		SetTitle( status.c_str() );
	}

	void ListSelection()
	{
		//sepaline();
		std::cout << "\nin this selection:" << std::endl;
		rsFileCategories fileStat;
		fileStat.Count( selection );
		print( fileStat );
	}

	void go()
	{
		std::string cmd;
		std::wstring params;
		while( true )
		{
			Header();
			sepaline();
			GetCmd( cmd, params );

			try
			{
				if( cmd == "close" )
				{
					save();
					return;
				} else

				if( cmd == "help" || cmd == "h" )
				{
					help( params, true );
				} else

				if( cmd == "autohelp" )
				{
					autoHelp = !autoHelp;
					std::cout << (autoHelp ? "will" : "won't") << " show relevant commands after your actions" << std::endl;
				} else

				if( cmd == "font" )
				{
					ChangeFonts();
					std::cout << "ok" << std::endl;
				} else

				if( cmd == "dirty" )
				{
					ecfArchive.dirty = true;
					std::cout << "ok" << std::endl;
				} else

				if( cmd == "save" )
				{
					save( params );
				} else

				if( cmd == "capture" || cmd == "c" )
				{
					capture( params );
					//fileStat: *?*?*?*; selectionStat: preferable; help: *?*?*?*
					help();
				} else

				if( cmd == "match" )//|| cmd == "m" )
				{
					match( params );
					//fileStat: preferable; selectionStat: useless; help: *?*?*?*
					help();
				} else

				if( cmd == "release" )//|| cmd == "rl" )
				{
					release( params );
					//fileStat: *?*?*?*; selectionStat: invalid; help: *?*?*?*
				} else

				if( cmd == "add" || cmd == "a" )
				{
					add( params, false );
					//fileStat: *?*?*?*; selectionStat: useless; help: preferable
					help();
				} else

				if( cmd == "addr" || cmd == "ar" )
				{
					add( params, true );
					//fileStat: *?*?*?*; selectionStat: useless; help: preferable
					help();
				} else

				if( cmd == "remove" )//|| cmd == "r" )
				{
					remove( params );
					//fileStat: *?*?*?*; selectionStat: useless; help: *?*?*?*
					help();
				} else

				if( cmd == "select" || cmd == "s" )
				{
					select( params );
					//fileStat: preferable; selectionStat: preferable; help: *?*?*?*
					if( selection.size() ) ListSelection();
					else help();
				} else

				if( cmd == "deselect" )
				{
					deselect( params );
					//fileStat: preferable; selectionStat: preferable; help: *?*?*?*
					if( selection.size() ) ListSelection();
					else help();
				} else

				if( cmd == "inverse" )
				{
					inverse( params );
					//fileStat: preferable; selectionStat: preferable; help: *?*?*?*
					if( selection.size() ) ListSelection();
					else help();
				} else

				if( cmd == "include" )
				{
					include( params );
					//fileStat: preferable; selectionStat: preferable; help: *?*?*?*
					if( selection.size() ) ListSelection();
					else help();
				} else

				if( cmd == "narrow" )
				{
					narrow( params );
					//fileStat: preferable; selectionStat: preferable; help: *?*?*?*
					if( selection.size() ) ListSelection();
					else help();
				} else

				if( cmd == "exclude" )
				{
					exclude( params );
					//fileStat: preferable; selectionStat: preferable; help: *?*?*?*
					if( selection.size() ) ListSelection();
					else help();
				} else

				if( cmd == "list" || cmd == "l" )
				{
					list( params );
					//fileStat: *?*?*?*; selectionStat: *?*?*?*; help: *?*?*?*
					help();
				} else

				if( cmd == "info" || cmd == "i" )
				{
					info( params );
					//fileStat: *?*?*?*; selectionStat: *?*?*?*; help: *?*?*?*
					help();
				} else

				if( cmd == "folders" || cmd == "f" )
				{
					folders( params );
					//fileStat: *?*?*?*; selectionStat: *?*?*?*; help: *?*?*?*
				} else

				if( cmd == "sort" )//|| cmd == "s" )
				{
					sort( params );
					//fileStat: useless; selectionStat: useless; help: useless
				} else

				if( cmd == "ecc" || cmd == "e" )
				{
					ecc( params );
					//fileStat: useless; selectionStat: preferable; help: preferable
					help();
				} else

				if( cmd == "md5" || cmd == "m" )
				{
					md5( params );
					//fileStat: useless; selectionStat: preferable; help: preferable
					help();
				} else

				if( cmd == "fix" /*|| cmd == "fix"*/ )
				{
					fix( params );
					//help();
				} else

				if( cmd == "update" || cmd == "up" )
				{
					update( params );
					//fileStat: *?*?*?*; selectionStat: preferable; help: *?*?*?*
					ListSelection();
				} else

				if( cmd == "uncopy" || cmd == "uc" )
				{
					uncopy( params );
					//fileStat: *?*?*?*; selectionStat: preferable; help: *?*?*?*
					ListSelection();
				} else

				if( cmd == "unmove" || cmd == "um" )
				{
					unmove( params );
					//fileStat: *?*?*?*; selectionStat: preferable; help: *?*?*?*
					ListSelection();
				} else

				if( cmd == "overwrite" )
				{
					overwrite( params );
					//fileStat: *?*?*?*; selectionStat: *?*?*?*; help: *?*?*?*
					ListSelection();
				} else

				if( cmd == "import" )
				{
					import( params );
					//fileStat: *?*?*?*; selectionStat: *?*?*?*; help: *?*?*?*
					help();
				} else

				if( cmd == "export" )
				{
					export_( params );
					//fileStat: *?*?*?*; selectionStat: *?*?*?*; help: *?*?*?*
				}
			}
			catch ( std::runtime_error& e )
			{
				std::cout << e.what() << std::endl;
			}
		}
	}

	void save( const std::wstring& ecfArchiveName = L"" )
	{
		if( ecfArchiveName.empty() )
		{
			if( ecfArchive.Dirty() )
			{
				std::cout << "save changes to '" << ecfArchive.fsName << "'?" << std::endl;
				if( ynPrompt() )
				{
					WriteEcfArchive( ecfArchive );
					ecfArchive.Clear();
					std::cout << "successfully saved" << std::endl;
				}
			}
			else
			{
				std::cout << "no changes been made to save" << std::endl;
			}
			return;
		}

		if( PathExists(ecfArchiveName) )
		{
			std::cout << "file '" << ecfArchiveName << "' already exists, overwrite?" << std::endl;
			if( !ynPrompt() ) return;
		}

		WriteEcfArchive( ecfArchive, ecfArchiveName );
		std::cout << "successfully saved" << std::endl;
	}

	void capture( const std::wstring& )
	{
		//todo: amount of captured files
		if( selection.size() )
			fileList.CaptureRealFiles( selection );
		else
			fileList.CaptureRealFiles();
		/*std::size_t amount = 
		if( amount )
			std::cout << "recaptured files: " << amount << std::endl;
		else
			std::cout << "no files were recaptured" << std::endl;*/
		std::cout << "done" << std::endl;
	}

	void match( const std::wstring& )
	{
		std::size_t count = fileList.size();
		fileList.MatchRealFilesToEc();
		count -= fileList.size();
		selection.clear();
		std::cout << "done: files matched: " << count << std::endl;
	}

	void release( const std::wstring& )
	{
		EnforceSelectionNotEmpty();

		fileList.DropRealFiles( selection );
		std::cout << "ok" << std::endl;
	}

	void add( const std::wstring& realPath, bool recursive )
	{
		std::vector<uniq_ptr<rsFile>> searchResult;

		if( realPath.empty() )
		{
			ListFiles( realPath, L"*", searchResult, recursive );
		}
		else if( FilePathMask(realPath) )
		{
			ListFiles( FilePathFromPath(realPath), FileNameFromPath(realPath), searchResult, recursive );
		}
		else
		{
			FILE_TYPE fileType = GetFileType( realPath );
			ENFORCE( fileType != FT_INVALID )("path '")(realPath)("' not found");

			if( fileType == FT_DIR )
			{
				ListFiles( realPath + L"\\", L"*", searchResult, recursive );
			}
			else
			{
				searchResult.push_back( ReadFileData(realPath) );
			}
		}

		// remove archive itself from the listing, if search includes root folder
		// bug: it is still possible to force add it via absolute path
		if( FilePathFromPath(realPath).empty() )
		{
			struct {
				const std::wstring name;
				bool operator () ( const uniq_ptr<rsFile>& realfile ) {
					return realfile->name == name;
				}
			} cmp = { FileNameFromPath(ecfArchive.fsName) };

			std::vector<uniq_ptr<rsFile>>::iterator it =
				std::find_if( searchResult.begin(), searchResult.end(), cmp );
			if( it != searchResult.end() ) searchResult.erase( it );
		}

		ENFORCE( searchResult.size() > 0 )("no files found");
		std::size_t existed = 0;
		std::size_t matched = 0;
		std::size_t added = 0;
		selection.clear();
		selection.reserve( searchResult.size() );
		fileList.reserve( fileList.size() + searchResult.size() );
		bool verbose = searchResult.size() < 30;

		for( std::size_t i = 0, e = searchResult.size(); i < e; i++ )
		{
			const std::wstring& realFileName = searchResult[i]->name;

			rsEcfArchiveManager::iterator it = fileList.Find( realFileName );
			if( it != fileList.end() )
			{
				if( verbose ) std::cout << "file '" << realFileName << "' already exists" << std::endl;
				existed++;
				continue;
			}

			it = fileList.AddRealIfMatches( searchResult[i] );
			if( it != fileList.end() )
			{
				if( verbose ) std::cout << "recognised existing renamed file '" << it->ecFile->name << "'" << std::endl;
				matched++;
			}
			else
			{
				it = fileList.AddRealFile( searchResult[i] );
				selection.push_back( &(*it) );
				if( verbose ) std::cout << "added file '" << realFileName << "'" << std::endl;
				added++;
			}
		}

		if( !verbose )
		{
			std::cout << std::endl;
			if( added ) std::cout << "added new files: " << added << std::endl;
			if( matched ) std::cout << "recognised existing files: " << matched << std::endl;
			if( existed ) std::cout << "already existing files: " << existed << std::endl;
		}
	}

	void remove( const std::wstring& fileName )
	{
		EnforceSelectionNotEmpty();
		fileList.Remove( selection );
		std::cout << "removed files: " << selection.size() << std::endl;
		selection.clear();
	}

	void select( const std::wstring& command )
	{
		if( command.empty() )
		{
			SelectionHelp( "select" );
			return;
		}

		selection.clear();
		SearchForPairs( command, selection );

		if( selection.size() )
			std::cout << "selected files: " << selection.size() << std::endl;
		else
			std::cout << "no files selected" << std::endl;
	}

	void deselect( const std::wstring& )
	{
		if( selection.size() )
			std::cout << "deselected" << std::endl;
		else
			std::cout << "no files selected" << std::endl;
		selection.clear();
	}

	void inverse( const std::wstring& )
	{
		std::vector<rsFilePair*> results;
		fileList.Search( All, results );
		ExistsInSortedSelection pred( selection );

		results.erase( std::remove_if(results.begin(), results.end(), pred), results.end() );
		selection.swap( results );

		if( selection.size() )
			std::cout << "selected files: " << selection.size() << std::endl;
		else
			std::cout << "no files selected" << std::endl;
	}

	void include( const std::wstring& command )
	{
		if( command.empty() )
		{
			SelectionHelp( "include" );
			return;
		}

		const std::size_t files = selection.size();
		SearchForPairs( command, selection );
		std::sort( selection.begin(), selection.end() );
		selection.erase( std::unique(selection.begin(), selection.end()), selection.end() );
		const std::size_t added = selection.size() - files;

		if( added )
			std::cout << "files selected: " << added << ", total: " << selection.size() << std::endl;
		else
			std::cout << "no more files selected" << std::endl;
	}

	void narrow( const std::wstring& command )
	{
		if( command.empty() )
		{
			SelectionHelp( "narrow" );
			return;
		}
		EnforceSelectionNotEmpty();

		std::vector<rsFilePair*> results;
		SearchForPairs( command, results );
		ExistsInSortedSelection pred( results );

		const std::size_t files = selection.size();
		selection.erase( std::remove_if(selection.begin(), selection.end(), std::not1(pred)), selection.end() );
		const std::size_t removed = files - selection.size();

		if( removed )
			std::cout << "files deselected: " << removed << ", remained: " << selection.size() << std::endl;
		else
			std::cout << "no files deselected" << std::endl;
	}

	void exclude( const std::wstring& command )
	{
		if( command.empty() )
		{
			SelectionHelp( "exclude" );
			return;
		}
		EnforceSelectionNotEmpty();

		std::vector<rsFilePair*> results;
		SearchForPairs( command, results );
		ExistsInSortedSelection pred( results );

		const std::size_t files = selection.size();
		selection.erase( std::remove_if(selection.begin(), selection.end(), pred), selection.end() );
		const std::size_t removed = files - selection.size();

		if( removed )
			std::cout << "files deselected: " << removed << ", remained: " << selection.size() << std::endl;
		else
			std::cout << "no files deselected" << std::endl;
	}

	void list( const std::wstring& )
	{
		EnforceSelectionNotEmpty();

		sepaline();
		for( std::size_t i = 0, e = selection.size(); i < e; i++ )
		{
			NOT_EMPTY_PAIR( (*selection[i]) );

			if( selection[i]->realFile )
				std::cout << selection[i]->realFile->name;
			else
				std::cout << selection[i]->ecFile->name;

			std::cout << " - " <<
					rsFileCategories::Cat2Name(selection[i]->diff) << std::endl;
		}
	}

	void info( const std::wstring& )
	{
		EnforceSelectionNotEmpty();

		sepaline();
		for( std::size_t i = 0, e = selection.size(); i < e; i++ )
		{
			if( i ) std::cout << " - - - " << std::endl;
			print( *selection[i] );
		}
	}

	void folders( const std::wstring& )
	{
		EnforceSelectionNotEmpty();

		struct dir {
			dir() : size(0), files(0) {}
			uint64 size;
			std::size_t files;
		};
		std::map<std::wstring, dir> listing;

		for( std::size_t i = 0, e = selection.size(); i < e; i++ )
		{
			if( !selection[i]->ecFile ) continue;
			dir& d = listing[ FilePathFromPath(selection[i]->ecFile->name) ];
			d.size += selection[i]->ecFile->fileSize;
			d.files++;
		}

		std::map<std::wstring, dir>::iterator i = listing.begin();
		std::map<std::wstring, dir>::iterator e = listing.end();
		if( i == e )
		{
			std::cout << "no files with stored data" << std::endl;
		}
		else
		for( ; i != e; i++ )
		{
			std::cout << i->first << " - " << i->second.files << " files, " <<
				FileSizeToString(i->second.size) << std::endl;
		}
	}

	void sort( const std::wstring& )
	{
		selection.clear();
		fileList.Sort();
		std::cout << "done" << std::endl;
	}

	void ecc( const std::wstring& param )
	{
		EnforceSelectionNotEmpty();
		rsData::DATA_TYPE targetType;
		bool autoType = false;

		if( param.empty() ) {
			autoType = true;
		} else
		if( param == L"0" ) {
			targetType = rsData::DT_32kbRAW_1mbECC;
		} else
		if( param == L"1" ) {
			targetType = rsData::DT_12kbECC_1mbDATA;
		} else
		{
			ENFORCE( false )("todo: propper manual" );
		}

		CounterSetTarget( selection.size(), GetSelectionMass(),
			"  - error correction code calculation" );
		std::size_t count = 0;

		for( std::size_t i = 0, e = selection.size(); i < e; i++ )
		{
			if( selection[i]->realFile )
			{
				rsFile& realFile = *selection[i]->realFile;
				std::cout << realFile.name;

				if( realFile.data.Size() && autoType )
				{
					std::cout << " - already calculated" << std::endl;
					CounterAdvanceMass( realFile.fileSize );
				}
				else try
				{
					rsData::DATA_TYPE type = rsData::DT_NONE;
					if( !autoType ) {
						type = targetType;
					} else
					if( selection[i]->ecFile ) {
						type = selection[i]->ecFile->data.dataType;
					}
					if( type == rsData::DT_NONE ) {
						type = rsData::DT_DEFAULT;
					}

					std::cout << "..." << std::endl;
					const std::string oldMd5 = realFile.md5String;
					CalculateEcc( realFile, type );
					selection[i]->Update();

					CheckMd5Deviation( realFile, oldMd5 );
					count++;
				} catch ( std::runtime_error& e ) {
					std::cout << e.what() << std::endl;
					CounterAdvanceMass( realFile.fileSize );
				}
			}
			else
			{
				NOT_EMPTY_PAIR( (*selection[i]) );
				std::cout << "file '" << selection[i]->ecFile->name <<
					"' is orphan" << std::endl;
				CounterAdvanceMass( selection[i]->ecFile->fileSize );
			}
			CounterAdvanceCount( 1 );
		}
		std::cout << "done: files calculated: " << count << std::endl;
	}

	void md5( const std::wstring& param )
	{
		EnforceSelectionNotEmpty();
		const bool forceRecalc = !param.empty();

		CounterSetTarget( selection.size(), GetSelectionMass(),
			"  - md5 calculation" );
		std::size_t count = 0;

		for( std::size_t i = 0, e = selection.size(); i < e; i++ )
		{
			if( selection[i]->realFile )
			{
				rsFile& realFile = *selection[i]->realFile;
				std::cout << realFile.name;

				if( realFile.md5String.size() && !forceRecalc )
				{
					std::cout << " - already calculated" << std::endl;
					CounterAdvanceMass( realFile.fileSize );
				}
				else try
				{
					std::cout << "..." << std::endl;
					const std::string oldMd5 = realFile.md5String;
					CalculateMD5( realFile );

					CheckMd5Deviation( realFile, oldMd5 );
					realFile.data.dataType = rsData::DT_NONE;
					realFile.data.data.clear();
					selection[i]->Update();
					count++;
				} catch ( std::runtime_error& e ) {
					std::cout << e.what() << std::endl;
					CounterAdvanceMass( realFile.fileSize );
				}
			}
			else
			{
				NOT_EMPTY_PAIR( (*selection[i]) );
				std::cout << "file '" << selection[i]->ecFile->name <<
					"' is orphan" << std::endl;
				CounterAdvanceMass( selection[i]->ecFile->fileSize );
			}
			CounterAdvanceCount( 1 );
		}
		std::cout << "done: files calculated: " << count << std::endl;
	}

	void update( const std::wstring& )
	{
		std::size_t created = 0;
		std::size_t renamed = 0;
		std::size_t dated = 0;
		std::size_t changed = 0;
		std::size_t ecc = 0;
		std::size_t md5 = 0;
		for( std::size_t i = 0, e = selection.size(); i < e; i++ )
		{
			rsFilePair& pair = *selection[i];
			if( !pair.realFile )
			{
				NOT_EMPTY_PAIR( pair );
				//std::cout << "'" << selection[i]->ecFile->name << "' is orphan, nothing to store from" << std::endl;
				continue;
			}
			else if( !pair.ecFile )
			{
				fileList.StoreEcFile( pair );
				created++;
			}
			else
			{
				const rsFileComparison& cmp = pair.diff;
				if( !cmp.EqualSize() )
				{
					std::cout << "'" << pair.ecFile->name << "' backup is not compatible" << std::endl;
					continue;
				}

				if( cmp.IsMoved() )
				{
					pair.ecFile->name = pair.realFile->name;
					pair.ecFile->dirty = true;
					renamed++;
				}

				if( !cmp.EqualCreate() || !cmp.EqualWrite() )
				{
					if( cmp.Md5Equal() )
					{
						pair.ecFile->createDate = pair.realFile->createDate;
						pair.ecFile->writeDate = pair.realFile->writeDate;
						dated++;
					}
					else
					{
						std::cout << "'" << pair.ecFile->name << "' creation and/or wtite date differs, cannot resolve without md5 check" << std::endl;
					}
				}

				if( cmp.Md5Calculated() )
				{
					if( cmp.Md5Differs() )
					{
						std::cout << "'" << pair.ecFile->name << "' has md5 mismatch" << std::endl;
						continue;
					}

					if( !cmp.Md5Stored() )
					{
						pair.ecFile->md5String = pair.realFile->md5String;
						pair.ecFile->dirty = true;
						md5++;
					}

					if( cmp.EccCalculated() && (!cmp.EccStored() || !cmp.EccCompatible()) )
					{
						pair.ecFile->data = pair.realFile->data;
						pair.ecFile->dirty = true;
						ecc++;
					}
				}
			}

			pair.Update();
		}

		if( created || renamed || dated || changed || ecc || md5 )
		{
			std::cout << "done, backups - ";
			if( created ) std::cout << "created: " << created;
			if( created && renamed ) std::cout << "; ";
			if( renamed ) std::cout << "renamed: " << renamed;
			if( (created || renamed) && dated ) std::cout << "; ";
			if( dated ) std::cout << "changed dates: " << renamed;
			if( (created || renamed || dated) && changed ) std::cout << "; ";
			if( changed ) std::cout << "changed backup type: " << changed;
			if( (created || renamed || dated || changed) && ecc ) std::cout << "; ";
			if( ecc ) std::cout << "promoted to ecc: " << ecc;
			if( (created || renamed || dated || changed || ecc) && md5 ) std::cout << "; ";
			if( md5 ) std::cout << "promoted to md5: " << md5;
			std::cout << std::endl;
		}
		else
		{
			std::cout << "no files were updated" << std::endl;
		}
	}

	void uncopy( const std::wstring& cmd )
	{
		bool hackoverride = cmd.size() > 0;
		std::size_t count = 0;
		for( std::size_t i = 0, e = selection.size(); i < e; i++ )
		{
			if( !hackoverride ) {
				const rsFileComparison& cmp = selection[i]->diff;
				if( cmp.EqualCreate() && cmp.EqualWrite() ) continue;
				if( !cmp.EqualSize() ) continue;
				if( !(cmp.IsCopied() || cmp.Md5Equal()) ) continue;
			}

			try {
				rsFilePair& pair = *selection[i];
				SetFileTime( *pair.realFile,
					pair.ecFile->createDate, pair.ecFile->writeDate );
				pair.Update();
				count++;
			} catch ( std::runtime_error& e ) {
				std::cout << e.what() << std::endl;
			}
		}
		std::cout << "done: dates restored: " << count << std::endl;
	}

	void unmove( const std::wstring& )
	{
		std::size_t count = 0;
		for( std::size_t i = 0, e = selection.size(); i < e; i++ )
		{
			const rsFileComparison& cmp = selection[i]->diff;
			if( cmp.EqualMeta() || cmp.EqualName() ) continue;
			if( !(cmp.IsMoved() || cmp.Md5Equal()) ) continue;

			try {
				////////////////////////////////////////////////////////////////////////////////////////////
				std::cout << "trying to move a file, failing due to function not implemented!" << std::endl;
				////////////////////////////////////////////////////////////////////////////////////////////
				//pair.Update();
				count++;
			} catch ( std::runtime_error& e ) {
				std::cout << e.what() << std::endl;
			}
		}
		std::cout << "done: file paths restored: " << count << std::endl;
	}

	void overwrite( const std::wstring& )
	{
		std::size_t count = 0;
		for( std::size_t i = 0, e = selection.size(); i < e; i++ )
		{
			if( !selection[i]->realFile ) continue;
			if( !selection[i]->ecFile ) continue;
			*selection[i]->ecFile = *selection[i]->realFile;
			selection[i]->Update();
			count++;
		}
		std::cout << "done: files overwritten: " << count << std::endl;
		ecfArchive.dirty |= count > 0;
	}

	void import( const std::wstring& ecfArchiveName )
	{
		ENFORCE( !ecfArchiveName.empty() )("supply a file name to add from");

		rsEcfArchive ecfImport( ecfArchiveName );
		ReadEcfArchive( ecfImport );

		std::size_t added = 0;
		std::size_t notadded = 0;
		for( std::size_t i = 0, e = ecfImport.ecFiles.size(); i < e; ++i )
		{
			if( fileList.Find(ecfImport.ecFiles[i]->name) == fileList.end() )
			{
				fileList.AddEcFile( ecfImport.ecFiles[i] );
				added++;
			}
			else
			{
				std::cout << "cannot import '" <<
					ecfImport.ecFiles[i]->name <<
					"', it already exists" << std::endl;
				notadded++;
			}
		}

		if( added )
		{
			//todo: select added files
			//const std::size_t files = selection.size();
			//it = fileList.AddRealFile( searchResult[i] );
			//selection.push_back( &(*it) );
			selection.clear();
			std::cout << "files added: " << added << std::endl;
		}
		else
		{
			std::cout << "no files added" << std::endl;
		}
		if( notadded )
			std::cout << "files not added: " << notadded << std::endl;
	}

	void export_( const std::wstring& ecfArchiveName )
	{
		EnforceSelectionNotEmpty();
		ENFORCE( !ecfArchiveName.empty() )("supply a file name to save to");

		rsEcfArchive ecfExport( ecfArchiveName );
		bool existing = false;
		try
		{
			ReadEcfArchive( ecfExport );
			std::cout << "specified archive already exists, add files to it?" << std::endl;
			if( !ynPrompt() ) return;
			std::cout << "adding files to existing archive..." << std::endl;
			existing = true;
		}
		catch( std::runtime_error& )
		{
			if( PathExists(ecfArchiveName) )
			{
				std::cout << "file '" << ecfArchiveName << "' exists, but is not an ecf archive, overwrite?" << std::endl;
				if( !ynPrompt() ) return;
			}
			WriteEcfArchive( ecfExport );
			std::cout << "exporting files to the new archive..." << std::endl;
		}

		std::size_t added = 0;
		std::size_t skipped = 0;
		std::size_t backupless = 0;
		rsEcfArchiveManager exporter( ecfExport );
		for( std::size_t i = 0, e = selection.size(); i < e; ++i )
		{
			if( !selection[i]->ecFile ) {
				NOT_EMPTY_PAIR( (*selection[i]) );
				std::cout << "cannot export '" << selection[i]->realFile->name <<
					"', it doesn't have backup" << std::endl;
				backupless++;
				continue;
			}

			if( existing && exporter.Find(selection[i]->ecFile->name) != exporter.end() ) {
				std::cout << "cannot export '" << selection[i]->ecFile->name <<
					"', it already exists in target" << std::endl;
				skipped++;
				continue;
			}

			uniq_ptr<rsFile> ecFile = new rsFile();
			*ecFile = *selection[i]->ecFile;
			ecfExport.ecFiles.push_back( ecFile );
			added++;
		}

		if( added )
			std::cout << "files exported: " << added << std::endl;
		else
			std::cout << "no files exported" << std::endl;
		if( skipped )
			std::cout << "files skipped due to them already existing in target: " << skipped << std::endl;
		if( backupless )
			std::cout << "files skipped due to lack of their backup: " << backupless << std::endl;

		if( added ) WriteEcfArchive( ecfExport );
	}

private:
	rsEcfArchive& ecfArchive;
	rsEcfArchiveManager fileList;
	std::vector<rsFilePair*> selection;

	void EnforceSelectionNotEmpty()
	{
		ENFORCE( selection.size() > 0 )
			("nothing selected to process, use 'select' first");
	}

	void CheckMd5Deviation( const rsFile& realFile, const std::string& oldMd5 )
	{
		ASSERT( !realFile.md5String.empty() )
			("why are you checking md5 deviation with no md5?");
		if( !oldMd5.empty() && realFile.md5String != oldMd5 )
		{
			std::cout << ">'" << realFile.name <<
				"' - md5 has changed on recalculation" << std::endl;
		}
	}

	void SelectionHelp( const char* cmd )
	{
		std::cout << "usage: '" << cmd << " cmd', where 'cmd' can be any of:" << std::endl;
		for( std::size_t i = 0; i < FCC_ENUM_SIZE; i++ ) {
			const FILE_COMPARISON_CATEGORY cat = FILE_COMPARISON_CATEGORY(i);
			std::cout << rsFileCategories::Cat2Name( cat ) << " ";
		}
		std::cout << "noecc ecc0 ecc1 all" << std::endl;
		std::cout << "or a plain file name/mask" << std::endl;
	}

	void SearchForPairs( const std::wstring& command, std::vector<rsFilePair*>& results )
	{
		const FILE_COMPARISON_CATEGORY cat = rsFileCategories::Name2Cat( WtoA(command) );
		if( cat != FCC_ENUM_SIZE ) {
			fileList.Search( Categorized(cat), results );
		} else
		if( command == L"noecc" ) {
			fileList.Search( NoEcc, results );
		} else
		if( command == L"ecc0" ) {
			fileList.Search( Ecc0, results );
		} else
		if( command == L"ecc1" ) {
			fileList.Search( Ecc1, results );
		} else
		if( command == L"all" ) {
			fileList.Search( All, results );
		} else
		{
			fileList.Search( Mask(command), results );
		}
	}

	uint64 GetSelectionMass()
	{
		uint64 mass = 0;
		for( std::size_t i = 0, e = selection.size(); i < e; ++i )
		{
			if( selection[i]->realFile )
				mass += selection[i]->realFile->fileSize;
			else if( selection[i]->ecFile )
				mass += selection[i]->ecFile->fileSize;
		}
		return mass;
	}
};
