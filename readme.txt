notepadImage                                                                   2013-02-09

This tool is based on the sources from Stepho 2005
Additions are done by mattesh(at)gmx.net to ensure function on all windows systems 
including Windows 7. As well as adding the same to an installer program.
Convert to Notepad++ plugin by luoyonggang(at)gmail.com to getting it to be easily installed,
also make sure it can support for Unicode.

It's a little Notepad++ launcher which allows you to replace notepad.exe completely 
by Notepad++. (Without need for removing anything from the windows system.)

Concept:
      notepadImage make use of a debugger feature in Windows the system will call a hooked 
      process with appended parameters to allow debugging the intended application.
      This hook application will be call whenever the correct application was resolved.
      
      notepad.exe receives always only one parameter which is now just deferred to Notepad++.
      Because notepad.exe is a blocking executable, also notepadImage behaves blocking by 
      default. If you like to have it non-blocking just change the setting WaitForClose in 
      the registry. The blocked process will show either the first Notepad++ window and wait
      until the file get closed or even Notepad++ closes. 
      
Comments and contributions to mattesh(at)gmx.net 
