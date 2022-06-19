#include <iostream>
#include <fstream>
#include <algorithm>

#include "nbstd.h"
#include "rsUtils.h"
#include "rsPlatform.h"
#include "rsSchifra.h"

/*
 * Error Correction - restoring damaged data via it's ecc
 * Error Correction Code - schifra ecc stream
 * Error Correction File - backup file with ecc and metadata
 * Error Correction Files Archive - archive with one or more backup files
 */



//-----------------------------------------------------------------------------
bool ynPrompt()
{
	std::cout << "y/n(/c)> ";
	while( true )
	{
		std::string line;
		getline( std::cin, line );
		if( line == "y" ) return true;
		if( line == "n" ) return false;
		if( line == "c" ) throw std::runtime_error( "canceled" );
	}
}
void sepaline(){std::cout << "________________________" << std::endl;}



//-----------------------------------------------------------------------------
#include "rsEcfArchive.h"
#include "rsFileComparison.h"
#include "rsPrinters.h"
#include "MenuArchiveManager.h"
#include "MenuMain.h"
/* TODO:
 * �������� ����� ���� ������ ������ ��5 ����� ��������, ���� ����� ���� �� � ����,
	10� ��������� ������ ���������� � ��������� ��� ������ ��� ��� ����� ��
 * Also, for a single file, read cache can be flushed simply by opening the file with
	FILE_FLAG_NO_BUFFERING specified and closing the handle again.
 * ss.seekp( reserved ); ss.put('\0'); ss.seekp( 0 );
 * ������ ��� � �������� ����� ���� ������, ������ � ����� ������� ������, 
 * ���������� ��� � ������� ������� ������� ������ �� ��������� ������ ����� ������ �����, 
 * ����� ��� ������ �������� ��� ��� ���������� ��� � ��5 �������� �� ������
 * diff tag: 00000000000000000001011110110111 - ���� ����������� � ����� ���������
 * diff tag: 00000000000000000010111101111111 - ��5 ����� ��� ���� �������� ��� ���
 * diff tag: 00000000000000000010111011110111 - ����� � ��5 � �� �������� ����
 * diff tag: 00000000000000000001001100110011 - xz
 * diff tag: 00000000000000000001000100110011 - xz
 *           00000000000000000101000111111111 - ��� �����������, �� ��� ����
 * ������� �������� ��� ��� ��5 �� ������ ��� ���������� � �������
 * ���� ����/*.���� ����������
 * //������� �������� � ������ ����� - oobral misguided error
 * ��������� ����� ����
 * �������� ������ ����������
 * ifstream from( "E:\\Documents and Settings\\Administrator\\Desktop\\zaz" );
 * ofstream to( "E:\\Documents and Settings\\Administrator\\Desktop\\zaz", ios_base::out | ios_base::in );
 * ������� ������ ������ ���� ���� � ������ ����� ������ //but should it?
 * import should select imported files
 */



//-----------------------------------------------------------------------------
//#include "import/md5.h"
#include "H:/Library/C++/TESTESTERON/nbstd/asserter.cpp"
#include <clocale>
int wmain( int argc, wchar_t *argv[] )
{
#ifdef _DEBUGz
	system("nircmd win max etitle \"\\DerpEcc_d.exe\"");
#endif
/*
		void datetest(); datetest();
		if(false)
		try
		{
			std::string cmd;
			std::wstring params;
			GetCmd( cmd, params );
		}
		catch ( std::runtime_error& e )
		{
			std::cout << e.what() << std::endl;
		}
		system("pause");

/*/
	if( IsPipe() ) {
		//std wcin version works with russian only, windows version crashes piping
		std::setlocale(LC_ALL, "");
		//printf ("Locale is: %s\n", std::setlocale(LC_ALL,NULL) ); //Russian_Russia.1251
		ChangeFonts();
	}

	std::cout << "usage notes:" << std::endl;
	std::cout << "locked by another program files may show as not found" << std::endl;
	std::cout << "if you're low on memory, 'release' unused files after calculation" << std::endl;
	std::cout << "ecc ver: v1; archive ver: mkI" << std::endl;
	std::cout << std::endl;//sepaline();

	if( argc > 1 )
	{
		for( int i = 1; i < argc; ++i )
		{
			MenuMain menu( argv[i] );
		}
	}
	else
	{
		MenuMain();
	}
//*/

	return 0;
}
