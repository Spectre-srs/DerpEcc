// Main Menu - nekotoroya huyinya
// takes nothing
// opens ecfArchive and goes to MenuArchiveManager



//-----------------------------------------------------------------------------
class MenuMain
{
public:
	MenuMain()
	{
		go();
	}

	MenuMain( const std::wstring& ecfArchiveName )
	{
		try {
			open( ecfArchiveName );
		} catch ( std::runtime_error& e ) {
			std::cout << e.what() << std::endl;
			go();
		}
	}

	void help()
	{
		sepaline();
		std::cout << "ecf - open ecf archive" << std::endl;
		std::cout << "new - create ecf archive" << std::endl;
		std::cout << "quit - exit application" << std::endl;
	}

	void go()
	{
		help();
		std::string cmd;
		std::wstring params;
		while( true )
		{
			sepaline();
			SetTitle( "Error Correction File Utility" );
			GetCmd( cmd, params );

			try
			{
				if( cmd == "quit" || cmd == "exit" )
				{
					return;
				} else

				if( cmd == "ecf" || cmd == "e" || cmd == "open" || cmd == "o" )
				{
					open( params );
					help();
				} else

				if( cmd == "new" || cmd == "n" || cmd == "create" || cmd == "c" )
				{
					create( params );
					help();
				} else

				if( cmd == "font" )
				{
					ChangeFonts();
					std::cout << "ok" << std::endl;
				}
			}
			catch ( std::runtime_error& e )
			{
				std::cout << e.what() << std::endl;
			}
		}
	}

	void open( const std::wstring& ecfArchiveName )
	{
		ENFORCE( !ecfArchiveName.empty() )("supply a file name to open");

		rsEcfArchive ecfArchive( ecfArchiveName );
		ReadEcfArchive( ecfArchive );

		MenuArchiveManager menu( ecfArchive );
	}

	void create( const std::wstring& ecfArchiveName )
	{
		ENFORCE( !ecfArchiveName.empty() )("supply a file name to create");

		if( PathExists(ecfArchiveName) )
		{
			std::cout << "file '" << ecfArchiveName << "' already exists, overwrite?" << std::endl;
			if( !ynPrompt() ) return;
		}

		rsEcfArchive ecfArchive( ecfArchiveName );
		WriteEcfArchive( ecfArchive );

		MenuArchiveManager menu( ecfArchive );
	}
};
