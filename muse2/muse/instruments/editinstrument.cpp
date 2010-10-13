//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: editinstrument.cpp,v 1.2.2.6 2009/05/31 05:12:12 terminator356 Exp $
//
//  (C) Copyright 2003 Werner Schweer (ws@seh.de)
//=========================================================

#include <q3listbox.h>
//Added by qt3to4:
#include <QCloseEvent>
#include <stdio.h> 
#include <errno.h>
#include <qmessagebox.h>
#include <qspinbox.h>
#include <qcombobox.h>
#include <qlineedit.h>
#include <qcheckbox.h>
#include <qdir.h>
#include <qfileinfo.h>
#include <q3filedialog.h>
#include <qtoolbutton.h>
#include <q3popupmenu.h>
#include <qpushbutton.h>
#include <qtabwidget.h>
#include <qinputdialog.h>

#include "editinstrument.h"
#include "minstrument.h"
#include "globals.h"
#include "listitem.h"
#include "song.h"
#include "xml.h"
#include "midictrl.h"
#include "gconfig.h"

enum {
      COL_NAME = 0, COL_TYPE,
      COL_HNUM, COL_LNUM, COL_MIN, COL_MAX, COL_DEF
      };

//---------------------------------------------------------
//   EditInstrument
//---------------------------------------------------------

EditInstrument::EditInstrument(QWidget* parent, const char* name, Qt::WFlags fl)
   : EditInstrumentBase(parent, name, fl)
      {
      patchpopup = new Q3PopupMenu(patchButton);
      patchpopup->setCheckable(false);
      
      // populate instrument list
      // Populate common controller list.
      for(int i = 0; i < 128; ++i)
        listController->insertItem(midiCtrlName(i));
      
      oldMidiInstrument = 0;
      oldPatchItem = 0;
      for (iMidiInstrument i = midiInstruments.begin(); i != midiInstruments.end(); ++i) {
            // Imperfect, crude way of ignoring internal instruments, soft synths etc. If it has a gui, 
            //  it must be an internal instrument. But this will still allow some vst instruments (without a gui) 
            //  to show up in the list.
            // Hmm, try file path instead...
            //if((*i)->hasGui())
            if((*i)->filePath().isEmpty())
              continue;
              
            ListBoxData* item = new ListBoxData((*i)->iname());
            item->setData((void*)*i);
            instrumentList->insertItem(item);
            }
      if(instrumentList->item(0))
        instrumentList->setSelected(instrumentList->item(0), true);
      //oldMidiInstrument = (MidiInstrument*)((ListBoxData*)instrumentList->item(0))->data();
      //oldMidiInstrument = (ListBoxData*)instrumentList->item(0);
      //oldMidiInstrument = (ListBoxData*)instrumentList->selectedItem();
      
//      MidiInstrument* wip = (MidiInstrument*)((ListBoxData*)instrumentList->item(0))->data();
//      if(wip)
        // Assignment
//        workingInstrument.assign( *wip );
      
      
      connect(instrumentList, SIGNAL(selectionChanged()), SLOT(instrumentChanged()));
      connect(patchView, SIGNAL(selectionChanged()), SLOT(patchChanged()));
      
      //instrumentChanged();
      changeInstrument();
      
      //connect(listController, SIGNAL(selectionChanged()), SLOT(controllerChanged()));
      connect(viewController, SIGNAL(selectionChanged()), SLOT(controllerChanged()));
      
      //connect(instrumentName, SIGNAL(textChanged(const QString&)), SLOT(instrumentNameChanged(const QString&)));
      connect(instrumentName, SIGNAL(returnPressed()), SLOT(instrumentNameReturn()));
      connect(instrumentName, SIGNAL(lostFocus()), SLOT(instrumentNameReturn()));
      
      connect(patchNameEdit, SIGNAL(returnPressed()), SLOT(patchNameReturn()));
      connect(patchNameEdit, SIGNAL(lostFocus()), SLOT(patchNameReturn()));
      connect(patchDelete, SIGNAL(clicked()), SLOT(deletePatchClicked()));
      connect(patchNew, SIGNAL(clicked()), SLOT(newPatchClicked()));
      connect(patchNewGroup, SIGNAL(clicked()), SLOT(newGroupClicked()));
      //connect(newCategory, SIGNAL(clicked()), SLOT(newCategoryClicked()));
      
      connect(patchButton, SIGNAL(clicked()), SLOT(patchButtonClicked()));
      connect(defPatchH, SIGNAL(valueChanged(int)), SLOT(defPatchChanged(int)));
      connect(defPatchL, SIGNAL(valueChanged(int)), SLOT(defPatchChanged(int)));
      connect(defPatchProg, SIGNAL(valueChanged(int)), SLOT(defPatchChanged(int)));
      connect(deleteController, SIGNAL(clicked()), SLOT(deleteControllerClicked()));
      connect(newController, SIGNAL(clicked()), SLOT(newControllerClicked()));
      connect(addController, SIGNAL(clicked()), SLOT(addControllerClicked()));
      connect(listController, SIGNAL(doubleClicked(Q3ListBoxItem*)), SLOT(addControllerClicked()));
      connect(ctrlType,SIGNAL(activated(int)), SLOT(ctrlTypeChanged(int)));
      connect(ctrlName, SIGNAL(returnPressed()), SLOT(ctrlNameReturn()));
      connect(ctrlName, SIGNAL(lostFocus()), SLOT(ctrlNameReturn()));
      //connect(ctrlName, SIGNAL(textChanged(const QString&)), SLOT(ctrlNameChanged(const QString&)));
      connect(spinBoxHCtrlNo, SIGNAL(valueChanged(int)), SLOT(ctrlHNumChanged(int)));
      connect(spinBoxLCtrlNo, SIGNAL(valueChanged(int)), SLOT(ctrlLNumChanged(int)));
      connect(spinBoxMin, SIGNAL(valueChanged(int)), SLOT(ctrlMinChanged(int)));
      connect(spinBoxMax, SIGNAL(valueChanged(int)), SLOT(ctrlMaxChanged(int)));
      connect(spinBoxDefault, SIGNAL(valueChanged(int)), SLOT(ctrlDefaultChanged(int)));
      connect(nullParamSpinBoxH, SIGNAL(valueChanged(int)), SLOT(ctrlNullParamHChanged(int)));
      connect(nullParamSpinBoxL, SIGNAL(valueChanged(int)), SLOT(ctrlNullParamLChanged(int)));
      
      connect(tabWidget3, SIGNAL(currentChanged(QWidget*)), SLOT(tabChanged(QWidget*)));
      //connect(sysexList, SIGNAL(selectionChanged()), SLOT(sysexChanged()));
      //connect(deleteSysex, SIGNAL(clicked()), SLOT(deleteSysexClicked()));
      //connect(newSysex, SIGNAL(clicked()), SLOT(newSysexClicked()));
      }

//---------------------------------------------------------
//   helpWhatsThis
//---------------------------------------------------------

void EditInstrument::helpWhatsThis()
{
  whatsThis();
}

//---------------------------------------------------------
//   fileNew
//---------------------------------------------------------

void EditInstrument::fileNew()
      {
      // Allow these to update...
      instrumentNameReturn();
      patchNameReturn();
      ctrlNameReturn();
      
      for (int i = 1;; ++i) {
            QString s = QString("Instrument-%1").arg(i);
            bool found = false;
            for (iMidiInstrument i = midiInstruments.begin(); i != midiInstruments.end(); ++i) {
                  if (s == (*i)->iname()) {
                        found = true;
                        break;
                        }
                  }
            if (!found) {
                  //if(oldMidiInstrument)
                  //{
                        //MidiInstrument* oi = (MidiInstrument*)old->data(Qt::UserRole).value<void*>();
                        MidiInstrument* oi = 0;
                        if(oldMidiInstrument)
                          oi = (MidiInstrument*)oldMidiInstrument->data();
                        MidiInstrument* wip = &workingInstrument;
                        //checkDirty(oi);
                        //if(checkDirty(oi))
                        if(checkDirty(wip))
                        // No save was chosen. Restore the actual instrument name.
                        {
                          if(oi)
                          {
                            oldMidiInstrument->setText(oi->iname());
                            //workingInstrument.setIName(oi->iname());
                            
                            // No file path? Only a new unsaved instrument can do that. So delete it.
                            if(oi->filePath().isEmpty())
                              // Delete the list item and the instrument.
                              deleteInstrument(oldMidiInstrument);
                            
                            instrumentList->triggerUpdate(true);
                          }  
                        }
                        //else  
                        //{
                        //  if(oi)
                            // Save was chosen. Assign the working instrument to the actual instrument.
                        //    oi->assign(workingInstrument);
                        //}  
                        
                        //oi->setDirty(false);
                        workingInstrument.setDirty(false);
                  //}
                        
                  MidiInstrument* ni = new MidiInstrument(s);
                  //midiInstruments.append(ni);
                  midiInstruments.push_back(ni);
                  //QListWidgetItem* item = new QListWidgetItem(ni->iname());
                  //InstrumentListItem* item = new InstrumentListItem(ni->iname());
                  ListBoxData* item = new ListBoxData(ni->iname());
                  
                  //oldMidiInstrument = item;
                  workingInstrument.assign( *ni );
                  //workingInstrument.setDirty(false);
                  
                  //item->setText(ni->iname());
                  item->setData((void*)ni);
                  //QVariant v = qVariantFromValue((void*)(ni));
                  //item->setData(Qt::UserRole, v);
                  //instrumentList->addItem(item);
                  instrumentList->insertItem(item);
                  
                  oldMidiInstrument = 0;
                  
                  instrumentList->blockSignals(true);
                  instrumentList->setCurrentItem(item);
                  instrumentList->blockSignals(false);
                  
                  changeInstrument();
                  
                  // We have our new instrument! So set dirty to true.
                  workingInstrument.setDirty(true);
                  
                  break;
                  }
            }

      }

//---------------------------------------------------------
//   fileOpen
//---------------------------------------------------------

void EditInstrument::fileOpen()
      {
      // Allow these to update...
      //instrumentNameReturn();
      //patchNameReturn();
      //ctrlNameReturn();
      
      
      }

//---------------------------------------------------------
//   fileSave
//---------------------------------------------------------

void EditInstrument::fileSave()
{
      //if (instrument->filePath().isEmpty())
      if (workingInstrument.filePath().isEmpty())
      {
        //fileSaveAs();
        saveAs();
        return;
      }      
      
      // Do not allow a direct overwrite of a 'built-in' muse instrument.
      QFileInfo qfi(workingInstrument.filePath());
      if(qfi.dirPath(true) == museInstruments)
      {
        //fileSaveAs();
        saveAs();
        return;
      }
      
      //QFile f(instrument->filePath());
      //if (!f.open(QIODevice::WriteOnly)) {
      //FILE* f = fopen(instrument->filePath().latin1(), "w");
      FILE* f = fopen(workingInstrument.filePath().latin1(), "w");
      if(f == 0)
      {
        //fileSaveAs();
        saveAs();
        return;
      }  
      
      // Allow these to update...
      instrumentNameReturn();
      patchNameReturn();
      ctrlNameReturn();
      
      //f.close();
      if(fclose(f) != 0)
      {
        //QString s = QString("Creating file:\n") + instrument->filePath() + QString("\nfailed: ")
        QString s = QString("Creating file:\n") + workingInstrument.filePath() + QString("\nfailed: ")
          //+ f.errorString();
          + QString(strerror(errno) );
        //fprintf(stderr, "poll failed: %s\n", strerror(errno));
        QMessageBox::critical(this, tr("MusE: Create file failed"), s);
        return;
      }
      
      //if(fileSave(instrument, instrument->filePath()))
      //  instrument->setDirty(false);
      if(fileSave(&workingInstrument, workingInstrument.filePath()))
        workingInstrument.setDirty(false);
}

//---------------------------------------------------------
//   fileSave
//---------------------------------------------------------

bool EditInstrument::fileSave(MidiInstrument* instrument, const QString& name)
    {
      //QFile f(name);
      //if (!f.open(QIODevice::WriteOnly)) {
      //      QString s("Creating file failed: ");
      //      s += strerror(errno);
      //      QMessageBox::critical(this,
      //         tr("MusE: Create file failed"), s);
      //      return false;
      //      }
      //Xml xml(&f);
      
      FILE* f = fopen(name.ascii(), "w");
      if(f == 0)
      {
        //if(debugMsg)
        //  printf("READ IDF %s\n", fi->filePath().latin1());
        QString s("Creating file failed: ");
        s += QString(strerror(errno));
        QMessageBox::critical(this,
            tr("MusE: Create file failed"), s);
        return false;
      }
            
      Xml xml(f);
      
      updateInstrument(instrument);
      
      //instrument->write(xml);
      instrument->write(0, xml);
      
      // Assign the working instrument values to the actual current selected instrument...
      if(oldMidiInstrument)
      {
        MidiInstrument* oi = (MidiInstrument*)oldMidiInstrument->data();
        if(oi)
        {
          oi->assign(workingInstrument);
          
          // Now signal the rest of the app so stuff can change...
          song->update(SC_CONFIG | SC_MIDI_CONTROLLER);
          //song->update(SC_CONFIG | SC_MIDI_CONTROLLER | SC_MIDI_CONTROLLER_ADD);
        }  
      }
      
      //f.close();
      //if (f.error()) {
      if(fclose(f) != 0)
      {
            QString s = QString("Write File\n") + name + QString("\nfailed: ")
               //+ f.errorString();
               + QString(strerror(errno));
            //fprintf(stderr, "poll failed: %s\n", strerror(errno));
            QMessageBox::critical(this, tr("MusE: Write File failed"), s);
            return false;
      }
      return true;
    }

//---------------------------------------------------------
//   saveAs
//---------------------------------------------------------

void EditInstrument::saveAs()
    {
      // Allow these to update...
      instrumentNameReturn();
      patchNameReturn();
      ctrlNameReturn();
      
      //QListWidgetItem* item = instrumentList->currentItem();
//      ListBoxData* item = (ListBoxData*)instrumentList->selectedItem();
//      if (item == 0)
//            return;
      //MidiInstrument* instrument = (MidiInstrument*)item->data(Qt::UserRole).value<void*>();
//      MidiInstrument* instrument = (MidiInstrument*)item->data();
      
      //QString path = QDir::homePath() + "/" + config.instrumentPath;
      //QString path = QDir::homeDirPath() + "/" + museGlobalShare;
      //QString path = museInstruments;
      QString path = museUserInstruments;
      
      if(!QDir(museUserInstruments).exists())
      {
        if(QMessageBox::question(this,
            tr("MusE:"),
            tr("The user instrument directory\n") + museUserInstruments + tr("\ndoes not exist yet. Create it now?\n") +
              tr("(You can override with the environment variable MUSEINSTRUMENTS)"),
            QMessageBox::Ok | QMessageBox::Default,
            QMessageBox::Cancel | QMessageBox::Escape,
            Qt::NoButton) == QMessageBox::Ok)
        {
          if(QDir().mkdir(museUserInstruments))
            printf("Created user instrument directory: %s\n", museUserInstruments.latin1());
          else
          {
            printf("Unable to create user instrument directory: %s\n", museUserInstruments.latin1());
            QMessageBox::critical(this, tr("MusE:"), tr("Unable to create user instrument directory\n") + museUserInstruments);
            //return;
            path = museUser;
          }
        } 
        else 
        //  return;  
          path = museUser;
      }
        
      //if (instrument->filePath().isEmpty())
      if (workingInstrument.filePath().isEmpty())
            path += QString("/%1.idf").arg(workingInstrument.iname());
      else {
            //QFileInfo fi(instrument->filePath());
            QFileInfo fi(workingInstrument.filePath());
            
            // Prompt for a new instrument name if the name has not been changed, to avoid duplicates.
            if(oldMidiInstrument)
            {
              MidiInstrument* oi = (MidiInstrument*)oldMidiInstrument->data();
              if(oi)
              {
                if(oi->iname() == workingInstrument.iname())
                {
                  // Prompt only if it's a user instrument, to avoid duplicates in the user instrument dir.
                  // This will still allow a user instrument to override a built-in instrument with the same name.
                  if(fi.dirPath(true) != museInstruments)
                  {
                    //QMessageBox::critical(this,
                    //    tr("MusE: Bad instrument name"),
                    //    tr("Please change the instrument name to a new unique name before saving, to avoid duplicates"),
                    //    QMessageBox::Ok,
                    //    QMessageBox::NoButton,
                    //    QMessageBox::NoButton);
                    printf("EditInstrument::saveAs Error: Instrument name %s already used!\n", workingInstrument.iname().latin1());
                    return;    
                  }  
                }
              }
            }  
            path += QString("/%1.idf").arg(fi.baseName());
           }
      //QString s = QFileDialog::getSaveFileName(this,
      //   tr("MusE: Save Instrument Definition"),
      //   path,
      //   tr("Instrument Definition (*.idf)"));
      
      QString s = Q3FileDialog::getSaveFileName(path, tr("Instrument Definition (*.idf)"), this,
         tr("MusE: Save Instrument Definition").latin1());
      if (s.isEmpty())
            return;
      //instrument->setFilePath(s);
      workingInstrument.setFilePath(s);
      
      //if (fileSave(instrument, s))
      //      instrument->setDirty(false);
      if(fileSave(&workingInstrument, s))
        workingInstrument.setDirty(false);
    }

//---------------------------------------------------------
//   fileSaveAs
//---------------------------------------------------------

void EditInstrument::fileSaveAs()
    {
      // Is this a new unsaved instrument? Just do a normal save.
      if(workingInstrument.filePath().isEmpty())
      {
        saveAs();
        return;
      }      
      
      // Allow these to update...
      instrumentNameReturn();
      patchNameReturn();
      ctrlNameReturn();
      
      MidiInstrument* oi = 0;
      if(oldMidiInstrument)
        oi = (MidiInstrument*)oldMidiInstrument->data();
        
      int res = checkDirty(&workingInstrument, true);
      switch(res)
      {
        // No save:
        case 1:
          //item->setText(instrument->iname());
          //instrumentList->triggerUpdate(true);
          //instrument->setDirty(false);
          workingInstrument.setDirty(false);
          if(oi)
          {
            oldMidiInstrument->setText(oi->iname());
            //workingInstrument.setIName(oi->iname());
            
            //workingInstrument.assign(*oi);
            
            // No file path? Only a new unsaved instrument can do that. So delete it.
            if(oi->filePath().isEmpty())
            {
              // Delete the list item and the instrument.
              deleteInstrument(oldMidiInstrument);
              oldMidiInstrument = 0;
            }
            
            changeInstrument();
            
            instrumentList->triggerUpdate(true);
          }
          return;
        break;
        
        // Abort:
        case 2: 
          return;
        break;
          
        // Save:
        case 0:
            //if(oi)
            //  oi->assign(workingInstrument);
            workingInstrument.setDirty(false);
        break;
      }
      
      //QListWidgetItem* item = instrumentList->currentItem();
//      ListBoxData* item = (ListBoxData*)instrumentList->selectedItem();
//      if (item == 0)
//            return;
      //MidiInstrument* instrument = (MidiInstrument*)item->data(Qt::UserRole).value<void*>();
//      MidiInstrument* instrument = (MidiInstrument*)item->data();
      
      bool isuser = false;
      QString so;
      if(workingInstrument.iname().isEmpty())
        so = QString("Instrument");
      else  
        so = workingInstrument.iname();
        
      for(int i = 1;; ++i) 
      {
        QString s = so + QString("-%1").arg(i);
        
        bool found = false;
        for(iMidiInstrument imi = midiInstruments.begin(); imi != midiInstruments.end(); ++imi) 
        {
          if(s == (*imi)->iname()) 
          {
            // Allow override of built-in instrument names.
            QFileInfo fi((*imi)->filePath());
            if(fi.dirPath(true) == museInstruments)
              break;
            found = true;
            break;
          }
        }
        if(found) 
          continue;  
        
        bool ok;
        s = QInputDialog::getText(tr("MusE: Save instrument as"), tr("Enter a new unique instrument name:"), 
                                  QLineEdit::Normal, s, &ok, this);
        if(!ok) 
          return;
        if(s.isEmpty())
        {
          --i;
          continue;  
        }  
          
        isuser = false;
        bool builtin = false;
        found = false;
        for(iMidiInstrument imi = midiInstruments.begin(); imi != midiInstruments.end(); ++imi) 
        {
          // If an instrument with the same name is found...
          if((*imi)->iname() == s) 
          {
            // If it's not the same name as the working instrument, and it's not an internal instrument (soft synth etc.)...
            if(s != workingInstrument.iname() && !(*imi)->filePath().isEmpty())
            {
              QFileInfo fi((*imi)->filePath());
              // Allow override of built-in and user instruments:
              // If it's a user instrument, not a built-in instrument...
              if(fi.dirPath(true) == museUserInstruments)
              {
                // No choice really but to overwrite the destination instrument file!
                // Can not have two user files containing the same instrument name.
                if(QMessageBox::question(this,
                    tr("MusE: Save instrument as"),
                    tr("The user instrument:\n") + s + tr("\nalready exists. This will overwrite its .idf instrument file.\nAre you sure?"),
                    QMessageBox::Ok | QMessageBox::Default,
                    QMessageBox::Cancel | QMessageBox::Escape,
                    Qt::NoButton) == QMessageBox::Ok)
                {
                  // Set the working instrument's file path to the found instrument's path.
                  workingInstrument.setFilePath((*imi)->filePath());
                  // Mark as overwriting a user instrument.
                  isuser = true;
                }  
                else
                {
                  found = true;
                  break;
                }
              }
              // Assign the found instrument to the working instrument.
              //workingInstrument.assign(*(*imi));
              // Assign the found instrument name to the working instrument name.
              workingInstrument.setIName(s);
              
              // Find the instrument in the list and set the old instrument to the item.
              oldMidiInstrument = (ListBoxData*)instrumentList->findItem(s, Q3ListBox::ExactMatch);
              
              // Mark as a built-in instrument.
              builtin = true;
              break;
            }  
            found = true;
            break;
          }
        }
        if(found)
        { 
          so = s;
          i = 0;
          continue;  
        }  
        
        so = s;
        
        // If the name does not belong to a built-in instrument...
        if(!builtin)
        {
          MidiInstrument* ni = new MidiInstrument();
          ni->assign(workingInstrument);
          ni->setIName(so);
          ni->setFilePath(QString());
          //midiInstruments.append(ni);
          midiInstruments.push_back(ni);
          //QListWidgetItem* item = new QListWidgetItem(ni->iname());
          //InstrumentListItem* item = new InstrumentListItem(ni->iname());
          //ListBoxData* item = new ListBoxData(ni->iname());
          ListBoxData* item = new ListBoxData(so);
          
          //oldMidiInstrument = item;
          workingInstrument.assign( *ni );
          //workingInstrument.setDirty(false);
          
          //item->setText(ni->iname());
          item->setData((void*)ni);
          //QVariant v = qVariantFromValue((void*)(ni));
          //item->setData(Qt::UserRole, v);
          //instrumentList->addItem(item);
          instrumentList->insertItem(item);
          
          oldMidiInstrument = 0;
          
          instrumentList->blockSignals(true);
          instrumentList->setCurrentItem(item);
          instrumentList->blockSignals(false);
          
          changeInstrument();
          
          // We have our new instrument! So set dirty to true.
          workingInstrument.setDirty(true);
        }  
          
        break;
      }
      
      //QString path = QDir::homePath() + "/" + config.instrumentPath;
      //QString path = QDir::homeDirPath() + "/" + museGlobalShare;
      //QString path = museInstruments;
      QString path = museUserInstruments;
      
      if(!QDir(museUserInstruments).exists())
      {
        if(QMessageBox::question(this,
            tr("MusE:"),
            tr("The user instrument directory\n") + museUserInstruments + tr("\ndoes not exist yet. Create it now?\n") +
              tr("(You can override with the environment variable MUSEINSTRUMENTS)"),
            QMessageBox::Ok | QMessageBox::Default,
            QMessageBox::Cancel | QMessageBox::Escape,
            Qt::NoButton) == QMessageBox::Ok)
        {
          if(QDir().mkdir(museUserInstruments))
            printf("Created user instrument directory: %s\n", museUserInstruments.latin1());
          else
          {
            printf("Unable to create user instrument directory: %s\n", museUserInstruments.latin1());
            QMessageBox::critical(this, tr("MusE:"), tr("Unable to create user instrument directory\n") + museUserInstruments);
            //return;
            path = museUser;
          }
        } 
        else 
        //  return;  
          path = museUser;
      }
      path += QString("/%1.idf").arg(so);
      
      //QString s = QFileDialog::getSaveFileName(this,
      //   tr("MusE: Save Instrument Definition"),
      //   path,
      //   tr("Instrument Definition (*.idf)"));
      
      QString sfn;
      // If we are overwriting a user instrument just force the path.
      if(isuser)
        sfn = path;
      else  
      {
        sfn = Q3FileDialog::getSaveFileName(path, tr("Instrument Definition (*.idf)"), this,
          tr("MusE: Save Instrument Definition").latin1());
        if (sfn.isEmpty())
              return;
        //instrument->setFilePath(s);
        workingInstrument.setFilePath(sfn);
      }  
      
      //if (fileSave(instrument, s))
      //      instrument->setDirty(false);
      if(fileSave(&workingInstrument, sfn))
        workingInstrument.setDirty(false);
    }

//---------------------------------------------------------
//   fileExit
//---------------------------------------------------------

void EditInstrument::fileExit()
      {

      }

//---------------------------------------------------------
//   closeEvent
//---------------------------------------------------------

void EditInstrument::closeEvent(QCloseEvent* ev)
      {
      // Allow these to update...
      instrumentNameReturn();
      patchNameReturn();
      ctrlNameReturn();
      
      //QListWidgetItem* item = instrumentList->currentItem();
      
//      ListBoxData* item = (ListBoxData*)instrumentList->selectedItem();
      
//      if(item)
//      {
        //MidiInstrument* instrument = (MidiInstrument*)item->data(Qt::UserRole).value<void*>();
//        MidiInstrument* instrument = (MidiInstrument*)item->data();
//        int res = checkDirty(instrument, true);
        MidiInstrument* oi = 0;
        if(oldMidiInstrument)
          oi = (MidiInstrument*)oldMidiInstrument->data();
          
        int res = checkDirty(&workingInstrument, true);
        switch(res)
        {
          // No save:
          case 1:
            //item->setText(instrument->iname());
            //instrumentList->triggerUpdate(true);
            //instrument->setDirty(false);
            workingInstrument.setDirty(false);
            if(oi)
            {
              oldMidiInstrument->setText(oi->iname());
              //workingInstrument.setIName(oi->iname());
              
              //workingInstrument.assign(*oi);
              
              // No file path? Only a new unsaved instrument can do that. So delete it.
              if(oi->filePath().isEmpty())
              {
                // Delete the list item and the instrument.
                deleteInstrument(oldMidiInstrument);
                oldMidiInstrument = 0;
              }
              
              changeInstrument();
              
              instrumentList->triggerUpdate(true);
            }  
          break;
          
          // Abort:
          case 2: 
            ev->ignore();
            return;
          break;
            
          // Save:
          case 0:
              //if(oi)
              //  oi->assign(workingInstrument);
              workingInstrument.setDirty(false);
          break;
          
        }
        
//      }
      
      Q3MainWindow::closeEvent(ev);
      }

//---------------------------------------------------------
//   changeInstrument
//---------------------------------------------------------

void EditInstrument::changeInstrument()
{
  ListBoxData* sel = (ListBoxData*)instrumentList->selectedItem();
  if(!sel)
    return;

  //oldMidiInstrument = (MidiInstrument*)sel->data();
  oldMidiInstrument = sel;
  // Assignment
  //workingInstrument = *((MidiInstrument*)sel->data());
  
  // Assign will 'delete' any existing patches, groups, or controllers.
  workingInstrument.assign( *((MidiInstrument*)sel->data()) );
  
  workingInstrument.setDirty(false);
  
  // populate patch list
  
  patchView->clear();
  //listController->clear();
  viewController->clear();
  //category->clear();
  //sysexList->clear();


  //MidiInstrument* instrument = (MidiInstrument*)sel->data(Qt::UserRole).value<void*>();
  //MidiInstrument* instrument = (MidiInstrument*)sel->data();
  //instrument->setDirty(false);

  instrumentName->blockSignals(true);
  //instrumentName->setText(instrument->iname());
  instrumentName->setText(workingInstrument.iname());
  instrumentName->blockSignals(false);
  
  nullParamSpinBoxH->blockSignals(true);
  nullParamSpinBoxL->blockSignals(true);
  int nv = workingInstrument.nullSendValue();
  if(nv == -1)
  {
    nullParamSpinBoxH->setValue(-1);
    nullParamSpinBoxL->setValue(-1);
  }  
  else
  {
    int nvh = (nv >> 8) & 0xff;
    int nvl = nv & 0xff;
    if(nvh == 0xff)
      nullParamSpinBoxH->setValue(-1);
    else  
      nullParamSpinBoxH->setValue(nvh & 0x7f);
    if(nvl == 0xff)
      nullParamSpinBoxL->setValue(-1);
    else  
      nullParamSpinBoxL->setValue(nvl & 0x7f);
  }
  nullParamSpinBoxH->blockSignals(false);
  nullParamSpinBoxL->blockSignals(false);
  
  //std::vector<PatchGroup>* pg = instrument->groups();
  //PatchGroupList* pg = instrument->groups();
  PatchGroupList* pg = workingInstrument.groups();
  //for (std::vector<PatchGroup>::iterator g = pg->begin(); g != pg->end(); ++g) {
  for (ciPatchGroup g = pg->begin(); g != pg->end(); ++g) {
        PatchGroup* pgp = *g; 
        if(pgp)
        {
          //QTreeWidgetItem* item = new QTreeWidgetItem;
          ListViewData* item = new ListViewData(patchView);
          
          //item->setText(0, g->name);
          item->setText(0, pgp->name);
          
          //QVariant v = QVariant::fromValue((void*)0);
          //item->setData(0, Qt::UserRole, v);
          //item->setData((void*)*g);
          //item->setData((void*)0);
          //item->setData((void*)&*g);
          item->setData((void*)pgp);
          //patchView->addTopLevelItem(item);
          
          //for (ciPatch p = g->patches.begin(); p != g->patches.end(); ++p) 
          for (ciPatch p = pgp->patches.begin(); p != pgp->patches.end(); ++p) 
          {
            //const Patch& patch = *p;
            Patch* patch = *p;
            if(patch)
            {
              //QTreeWidgetItem* sitem = new QTreeWidgetItem;
              ListViewData* sitem = new ListViewData(item);
              //sitem->setText(0, patch.name);
              //sitem->setText(0, p->name);
              sitem->setText(0, patch->name);
              //QVariant v = QVariant::fromValue((void*)patch);
              //sitem->setData(0, Qt::UserRole, v);
              //sitem->setData((void*)&*p);
              sitem->setData((void*)patch);
              //item->addChild(sitem);
            }  
          }  
        }
      }  
  //patchView->setSelected(patchView->item(0), true);
  
  oldPatchItem = 0;
  
  ListViewData* fc = (ListViewData*)patchView->firstChild();
  if(fc)
  {
    // This may cause a patchChanged call.
    //if(patchView->selectedItem() != fc)
    patchView->blockSignals(true);
    patchView->setSelected(fc, true);
    patchView->blockSignals(false);
    //else  
    //  patchChanged();
      
    //patchView->firstChild()->setSelected(true);
    //patchView->triggerUpdate(true);
  }
      
  patchChanged();
  
//      oldPatchItem = (ListViewData*)patchView->selectedItem();
      //patchChanged();
//      if(oldPatchItem)
//      {
//        if(oldPatchItem->parent())
//          patchNameEdit->setText( ((Patch*)oldPatchItem->data())->name );
//        else  
//          patchNameEdit->setText( ((PatchGroup*)oldPatchItem->data())->name );
//      }    
      
  //MidiControllerList* cl = instrument->controller();
  MidiControllerList* cl = workingInstrument.controller();
  for (ciMidiController ic = cl->begin(); ic != cl->end(); ++ic) {
        MidiController* c = ic->second;
        //QListWidgetItem* item = new QListWidgetItem(c->name());
     //   ListBoxData* item = new ListBoxData(c->name());
        //QVariant v = QVariant::fromValue((void*)c);
        //item->setData(Qt::UserRole, v);
    //    item->setData((void*)c);
    //    listController->insertItem(item);
        
        addControllerToView(c);
        }
  
  
  //listController->setItemSelected(listController->item(0), true);
  
//  oldController = 0;
  
  //ListBoxData* ci = (ListBoxData*)listController->item(0);
  ListViewData* ci = (ListViewData*)viewController->firstChild();
  
  if(ci)
  {
    // This may cause a controllerChanged call.
    //if(listController->selectedItem != ci)
  //  listController->blockSignals(true);
  //  listController->setSelected(ci, true);
  //  listController->blockSignals(false);
    //else  
    //  controllerChanged();
    
    viewController->blockSignals(true);
    viewController->setSelected(ci, true);
    viewController->blockSignals(false);
  }  
  
  controllerChanged();
  
  //oldController = (ListBoxData*)listController->selectedItem();
  
  
  //controllerChanged(listController->item(0), 0);
  //controllerChanged();
  
/*
      category->addItems(instrument->categories());

      foreach(const SysEx* s, instrument->sysex()) {
            QListWidgetItem* item = new QListWidgetItem(s->name);
            QVariant v = QVariant::fromValue((void*)s);
            item->setData(Qt::UserRole, v);
            sysexList->addItem(item);
            }

      sysexList->setItemSelected(sysexList->item(0), true);
      sysexChanged(sysexList->item(0), 0);

      if (!cl->empty()) {
            listController->setItemSelected(listController->item(0), true);
            controllerChanged(listController->item(0), 0);
            }
*/


}

//---------------------------------------------------------
//   instrumentChanged
//---------------------------------------------------------

void EditInstrument::instrumentChanged()
      {
      ListBoxData* sel = (ListBoxData*)instrumentList->selectedItem();
      if(!sel)
        return;
           
      //printf("instrument changed: %s\n", sel->text().latin1());
      
      //if (old) {
      //if(oldMidiInstrument)
      //{
        //MidiInstrument* oi = (MidiInstrument*)old->data(Qt::UserRole).value<void*>();
        MidiInstrument* oi = 0;
        if(oldMidiInstrument)
          oi = (MidiInstrument*)oldMidiInstrument->data();
        MidiInstrument* wip = &workingInstrument;
        // Returns true if aborted.
        //checkDirty(oi);
        //if(checkDirty(oi))
        if(checkDirty(wip))
        {
          // No save was chosen. Abandon changes, or delete if it is new...
          if(oi)
          {
            oldMidiInstrument->setText(oi->iname());
            //wip->setText(oi->iname());
            
            // No file path? Only a new unsaved instrument can do that. So delete it.
            if(oi->filePath().isEmpty())
            {
              // Delete the list item and the instrument.
              deleteInstrument(oldMidiInstrument);
              oldMidiInstrument = 0;
            }
            
            instrumentList->triggerUpdate(true);
          }  
        }
        //else
        //{
          // Save was chosen. 
        //  if(oi)
        //    oi->assign(workingInstrument);
        //}    
        
        //oi->setDirty(false);
        //wip->setDirty(false);
        workingInstrument.setDirty(false);
      //}

        changeInstrument();
        
      }

//---------------------------------------------------------
//   instrumentNameReturn
//---------------------------------------------------------

void EditInstrument::instrumentNameReturn()
//void EditInstrument::instrumentNameChanged(const QString& s)
{
  //instrumentNameChanged(instrumentName->text());
  ListBoxData* item = (ListBoxData*)instrumentList->selectedItem();
  if (item == 0)
        return;
  QString s = instrumentName->text();
  
  if(s == item->text()) 
    return;
  
  MidiInstrument* curins = (MidiInstrument*)item->data();
  
  for(iMidiInstrument i = midiInstruments.begin(); i != midiInstruments.end(); ++i) 
  {
    if((*i) != curins && s == (*i)->iname()) 
    {
      instrumentName->blockSignals(true);
      // Grab the last valid name from the item text, since the instrument has not been updated yet.
      //instrumentName->setText(curins->iname());
      instrumentName->setText(item->text());
      instrumentName->blockSignals(false);
      
      QMessageBox::critical(this,
          tr("MusE: Bad instrument name"),
          tr("Please choose a unique instrument name.\n(The name might be used by a hidden instrument.)"),
          QMessageBox::Ok,
          Qt::NoButton,
          Qt::NoButton);
          
      return;
    }
  }      
  
  //if (s != workingInstrument.iname()) {
        item->setText(s);
        //MidiInstrument* instrument = (MidiInstrument*)item->data(Qt::UserRole).value<void*>();
        //MidiInstrument* instrument = (MidiInstrument*)item->data();
        //instrument->setDirty(true);
        workingInstrument.setIName(s);
        workingInstrument.setDirty(true);
        //instrumentList->updateItem(item);
        instrumentList->triggerUpdate(true);
        //instrumentList->update();
  //      }
}

//---------------------------------------------------------
//   deleteInstrument
//---------------------------------------------------------

void EditInstrument::deleteInstrument(ListBoxData* item)
{
  if(!item)
    return;

  //ListBoxData* curritem = (ListBoxData*)instrumentList->selectedItem();
  
  MidiInstrument* ins = (MidiInstrument*)item->data();
  
  // Be kind to the list item, just in case we install a delete handler or something.
  //item->setData(0);
  
  // Delete the list item.
  // Test this: Is this going to change the current selection?
  instrumentList->blockSignals(true);
  delete item;
  instrumentList->blockSignals(false);
  
  // Test this: Neccessary?
  // if(curritem)
  //  instrumentList->setCurrentItem(curritem);
  
  if(!ins)
    return;
        
  // Remove the instrument from the list.
  midiInstruments.remove(ins);
  
  // Delete the instrument.
  delete ins;
}

//---------------------------------------------------------
//   tabChanged
//   Added so that patch list is updated when switching tabs, 
//    so that 'Program' default values and text are current in controller tab. 
//---------------------------------------------------------

void EditInstrument::tabChanged(QWidget* w)
{
  if(!w)
    return;
    
  // If we're switching to the Patches tab, just ignore.
  if(QString(w->name()) == QString("patchesTab"))
    return;
    
  if(oldPatchItem)
  {
    // Don't bother calling patchChanged, just update the patch or group.
    if(oldPatchItem->parent())
      updatePatch(&workingInstrument, (Patch*)oldPatchItem->data());
    else
      updatePatchGroup(&workingInstrument, (PatchGroup*)oldPatchItem->data());
  }
  
  // We're still on the same item. No need to set oldPatchItem as in patchChanged...
  
  // If we're switching to the Controller tab, update the default patch button text in case a patch changed...
  if(QString(w->name()) == QString("controllerTab"))
  {
    ListViewData* sel = (ListViewData*)viewController->selectedItem();
        
    if(!sel || !sel->data()) 
      return;
        
    MidiController* c = (MidiController*)sel->data();
    MidiController::ControllerType type = midiControllerType(c->num());
        
    // Grab the controller number from the actual values showing
    //  and set the patch button text.
    if(type == MidiController::Program)
      setDefaultPatchName(getDefaultPatchNumber());
  }  
}

//---------------------------------------------------------
//   patchNameReturn
//---------------------------------------------------------

void EditInstrument::patchNameReturn()
{
  ListViewData* item = (ListViewData*)patchView->selectedItem();
  if (item == 0)
        return;
  
  QString s = patchNameEdit->text();
  
  if(item->text(0) == s)
    return;
  
  PatchGroupList* pg = workingInstrument.groups();
  for(iPatchGroup g = pg->begin(); g != pg->end(); ++g) 
  {
    PatchGroup* pgp = *g;
    // If the item has a parent, it's a patch item.
    if(item->parent())
    {
      Patch* curp = (Patch*)item->data();
      for(iPatch p = pgp->patches.begin(); p != pgp->patches.end(); ++p) 
      {
        if((*p) != curp && (*p)->name == s) 
        {
          patchNameEdit->blockSignals(true);
          // Grab the last valid name from the item text, since the patch has not been updated yet.
          //patchNameEdit->setText(curp->name);
          patchNameEdit->setText(item->text(0));
          patchNameEdit->blockSignals(false);
          
          QMessageBox::critical(this,
              tr("MusE: Bad patch name"),
              tr("Please choose a unique patch name"),
              QMessageBox::Ok,
              Qt::NoButton,
              Qt::NoButton);
              
          return;
        }
      }  
    }
    else
    // The item has no parent. It's a patch group item.
    {
      PatchGroup* curpg = (PatchGroup*)item->data();
      if(pgp != curpg && pgp->name == s) 
      {
        patchNameEdit->blockSignals(true);
        // Grab the last valid name from the item text, since the patch group has not been updated yet.
        //patchNameEdit->setText(curpg->name);
        patchNameEdit->setText(item->text(0));
        patchNameEdit->blockSignals(false);
        
        QMessageBox::critical(this,
            tr("MusE: Bad patchgroup name"),
            tr("Please choose a unique patchgroup name"),
            QMessageBox::Ok,
            Qt::NoButton,
            Qt::NoButton);
            
        return;
      }
    }
  }
  
    item->setText(0, s);
    workingInstrument.setDirty(true);
    
  // Since the name of the patch/group in the working instrument will be updated later,
  //  there's no need to do manually set the name here now. 
  /*
  // If the item has a parent, it's a patch item.
  if(item->parent())
  {
    Patch* p = item->data();
    if(s != p->name)
    {
      item->setText(s);
      p->name = s;
      workingInstrument.setDirty(true);
      //patchView->triggerUpdate(true);
    }
  }
  else
  // The item has no parent. It's a patch group item.
  {
    PatchGroup* pg = (PatchGroup*)item->data();
    if(s != pg->name)
    {
      item->setText(s);
      pg->name = s;
      workingInstrument.setDirty(true);
      //patchView->triggerUpdate(true);
    }
  }
  */
}

//---------------------------------------------------------
//   patchChanged
//---------------------------------------------------------

void EditInstrument::patchChanged()
    {
      //if (old && old->data(0, Qt::UserRole).value<void*>()) {
      if(oldPatchItem)
      {
            //QListWidgetItem* item = instrumentList->currentItem();
            //if (item == 0)
            //      return;
            //MidiInstrument* instrument = (MidiInstrument*)item->data(Qt::UserRole).value<void*>();
            //Patch* p = (Patch*)old->data(0, Qt::UserRole).value<void*>();
            //updatePatch(instrument, p);
            if(oldPatchItem->parent())
              updatePatch(&workingInstrument, (Patch*)oldPatchItem->data());
            else
              updatePatchGroup(&workingInstrument, (PatchGroup*)oldPatchItem->data());
      }
      
      
      ListViewData* sel = (ListViewData*)patchView->selectedItem();
      oldPatchItem = sel;
      
      if(!sel || !sel->data()) 
      {
        patchNameEdit->setText("");
        spinBoxHBank->setEnabled(false);
        spinBoxLBank->setEnabled(false);
        spinBoxProgram->setEnabled(false);
        checkBoxDrum->setEnabled(false);
        checkBoxGM->setEnabled(false);
        checkBoxGS->setEnabled(false);
        checkBoxXG->setEnabled(false);
        return;
      }
      
      // If the item has a parent, it's a patch item.
      if(sel->parent())
      {
        Patch* p = (Patch*)sel->data();
        patchNameEdit->setText(p->name);
        spinBoxHBank->setEnabled(true);
        spinBoxLBank->setEnabled(true);
        spinBoxProgram->setEnabled(true);
        checkBoxDrum->setEnabled(true);
        checkBoxGM->setEnabled(true);
        checkBoxGS->setEnabled(true);
        checkBoxXG->setEnabled(true);
        
        int hb = ((p->hbank + 1) & 0xff);
        int lb = ((p->lbank + 1) & 0xff);
        int pr = ((p->prog + 1) & 0xff);
        spinBoxHBank->setValue(hb);
        spinBoxLBank->setValue(lb);
        spinBoxProgram->setValue(pr);
        //checkBoxDrum->setChecked(p->drumMap);
        checkBoxDrum->setChecked(p->drum);
        checkBoxGM->setChecked(p->typ & 1);
        checkBoxGS->setChecked(p->typ & 2);
        checkBoxXG->setChecked(p->typ & 4);
        //category->setCurrentIndex(p->categorie);
      }  
      else
      // The item is a patch group item.
      {
        patchNameEdit->setText( ((PatchGroup*)sel->data())->name );
        spinBoxHBank->setEnabled(false);
        spinBoxLBank->setEnabled(false);
        spinBoxProgram->setEnabled(false);
        checkBoxDrum->setEnabled(false);
        checkBoxGM->setEnabled(false);
        checkBoxGS->setEnabled(false);
        checkBoxXG->setEnabled(false);
      }  
    }

//---------------------------------------------------------
//   defPatchChanged
//---------------------------------------------------------

void EditInstrument::defPatchChanged(int)
{
      ListViewData* item = (ListViewData*)viewController->selectedItem();
      if (!item)
            return;
        
      MidiController* c = (MidiController*)item->data();
      
      int val = getDefaultPatchNumber();
      
      //if(val == c->minVal() - 1)
      //  c->setInitVal(CTRL_VAL_UNKNOWN);
      //else
        c->setInitVal(val);
      
      setDefaultPatchName(val);
      
      item->setText(COL_DEF, getPatchItemText(val));
      workingInstrument.setDirty(true);
}

//---------------------------------------------------------
//   patchButtonClicked
//---------------------------------------------------------

void EditInstrument::patchButtonClicked()
{
      //MidiTrack* track = (MidiTrack*)selected;
      //int channel = track->outChannel();
      //int port    = track->outPort();
      //MidiInstrument* instr = midiPorts[port].instrument();
      
      patchpopup->clear();

      PatchGroupList* pg = workingInstrument.groups();
      
      if (pg->size() > 1) {
            for (ciPatchGroup i = pg->begin(); i != pg->end(); ++i) {
                  PatchGroup* pgp = *i;
                  Q3PopupMenu* pm = new Q3PopupMenu(patchpopup);
                  pm->setCheckable(false);
                  pm->setFont(config.fonts[0]);
                  const PatchList& pl = pgp->patches;
                  for (ciPatch ipl = pl.begin(); ipl != pl.end(); ++ipl) {
                        const Patch* mp = *ipl;
                        //if ((mp->typ & mask) && 
                        //    ((drum && songType != MT_GM) || 
                        //    (mp->drum == drumchan)) )  
                            
                        //    {
                              int id = ((mp->hbank & 0xff) << 16)
                                         + ((mp->lbank & 0xff) << 8) + (mp->prog & 0xff);
                              pm->insertItem(mp->name, id);
                        //    }
                              
                        }
                  patchpopup->insertItem(pgp->name, pm);
                  }
            }
      else if (pg->size() == 1 ){
            // no groups
            const PatchList& pl = pg->front()->patches;
            for (ciPatch ipl = pl.begin(); ipl != pl.end(); ++ipl) {
                  const Patch* mp = *ipl;
                  //if (mp->typ & mask) {
                        int id = ((mp->hbank & 0xff) << 16)
                                 + ((mp->lbank & 0xff) << 8) + (mp->prog & 0xff);
                        patchpopup->insertItem(mp->name, id);
                  //      }
                  }
            }

      if(patchpopup->count() == 0)
        return;
        
      int rv = patchpopup->exec(patchButton->mapToGlobal(QPoint(10,5)));
      
      if (rv != -1) 
      {
        //if(rv != workingInstrument.
        
        setDefaultPatchControls(rv);
        
        ListViewData* item = (ListViewData*)viewController->selectedItem();
        if(item)
        {
          MidiController* c = (MidiController*)item->data();
          c->setInitVal(rv);
          
          item->setText(COL_DEF, getPatchItemText(rv));
        }
        workingInstrument.setDirty(true);
      }
            
}

//---------------------------------------------------------
//   addControllerToView
//---------------------------------------------------------

ListViewData* EditInstrument::addControllerToView(MidiController* mctrl)
{
      QString hnum;
      QString lnum;
      QString min;
      QString max;
      QString def;
      int defval = mctrl->initVal();
      int n = mctrl->num();
      //int h = (n >> 7) & 0x7f;
      int h = (n >> 8) & 0x7f;
      int l = n & 0x7f;
      if((n & 0xff) == 0xff)
        l = -1;
        
      MidiController::ControllerType t = midiControllerType(n);
      switch(t)
      {
          case MidiController::Controller7:
          //case MidiController::RPN:
          //case MidiController::NRPN:
                hnum = "---";
                if(l == -1)
                  lnum = "*";
                else  
                  lnum.setNum(l);
                min.setNum(mctrl->minVal());
                max.setNum(mctrl->maxVal());
                if(defval == CTRL_VAL_UNKNOWN)
                  def = "---";
                else
                  def.setNum(defval);
                break;
          case MidiController::RPN:
          case MidiController::NRPN:
          case MidiController::RPN14:
          case MidiController::NRPN14:
          case MidiController::Controller14:
                hnum.setNum(h);
                if(l == -1)
                  lnum = "*";
                else  
                  lnum.setNum(l);
                min.setNum(mctrl->minVal());
                max.setNum(mctrl->maxVal());
                if(defval == CTRL_VAL_UNKNOWN)
                  def = "---";
                else
                  def.setNum(defval);
                break;
          case MidiController::Pitch:
                hnum = "---";
                lnum = "---";
                min.setNum(mctrl->minVal());
                max.setNum(mctrl->maxVal());
                if(defval == CTRL_VAL_UNKNOWN)
                  def = "---";
                else
                  def.setNum(defval);
                break;
          case MidiController::Program:
                hnum = "---";
                lnum = "---";
                min = "---";
                max = "---";
                def = getPatchItemText(defval); 
                break;
                
          default:
                hnum = "---";
                lnum = "---";
                //min.setNum(0);
                //max.setNum(0);
                min = "---";
                max = "---";
                def = "---";
                break;
      }
      
      ListViewData* ci =  new ListViewData(viewController, mctrl->name(), int2ctrlType(t),
                                            hnum, lnum, min, max, def);
      ci->setData((void*)mctrl);
      //setModified(true);
      
      return ci;
}
      
//---------------------------------------------------------
//   controllerChanged
//---------------------------------------------------------

void EditInstrument::controllerChanged()
      {
      //if (old) {
//      if(oldController) 
//      {
            //QListWidgetItem* item = instrumentList->currentItem();
            //if (item == 0)
            //      return;
            //MidiInstrument* instrument = (MidiInstrument*)item->data(Qt::UserRole).value<void*>();
            //MidiController* oc = (MidiController*)old->data(Qt::UserRole).value<void*>();
            //updateController(instrument, oc);
//            updateController(&workingInstrument, (MidiController*)oldController->data());
//      }
      
    //  ListBoxData* sel = (ListBoxData*)listController->selectedItem();
      ListViewData* sel = (ListViewData*)viewController->selectedItem();
//      oldController = sel;
      
      if(!sel || !sel->data()) 
      {
        ctrlName->blockSignals(true);
        ctrlName->setText("");
        ctrlName->blockSignals(false);
        //ctrlComment->setText("");
        return;
      }
      
      //MidiController* c = (MidiController*)sel->data(Qt::UserRole).value<void*>();
      MidiController* c = (MidiController*)sel->data();
      
      ctrlName->blockSignals(true);
      ctrlName->setText(c->name());
      ctrlName->blockSignals(false);
      
      //ctrlComment->setText(c->comment());
      int ctrlH = (c->num() >> 8) & 0x7f;
      int ctrlL = c->num() & 0x7f;
      if((c->num() & 0xff) == 0xff)
        ctrlL = -1;
        
      //int type = int(c->type());
      MidiController::ControllerType type = midiControllerType(c->num());
      
      //ctrlType->setCurrentIndex(type);
      ctrlType->blockSignals(true);
      ctrlType->setCurrentItem(type);
      ctrlType->blockSignals(false);
      
      //ctrlTypeChanged(type);
      
      spinBoxHCtrlNo->blockSignals(true);
      spinBoxLCtrlNo->blockSignals(true);
      spinBoxMin->blockSignals(true);
      spinBoxMax->blockSignals(true);
      spinBoxDefault->blockSignals(true);
     
      //ctrlTypeChanged(type);
      
      switch (type) {
            //case MidiController::RPN:
            //case MidiController::NRPN:
            case MidiController::Controller7:
                  spinBoxHCtrlNo->setEnabled(false);
                  spinBoxLCtrlNo->setEnabled(true);
                  spinBoxHCtrlNo->setValue(0);
                  spinBoxLCtrlNo->setValue(ctrlL);
                  spinBoxMin->setEnabled(true);
                  spinBoxMax->setEnabled(true);
                  spinBoxMin->setRange(-128, 127);
                  spinBoxMax->setRange(-128, 127);
                  spinBoxMin->setValue(c->minVal());
                  spinBoxMax->setValue(c->maxVal());
                  enableDefaultControls(true, false);
                  break;
            case MidiController::RPN:
            case MidiController::NRPN:
                  spinBoxHCtrlNo->setEnabled(true);
                  spinBoxLCtrlNo->setEnabled(true);
                  spinBoxHCtrlNo->setValue(ctrlH);
                  spinBoxLCtrlNo->setValue(ctrlL);
                  spinBoxMin->setEnabled(true);
                  spinBoxMax->setEnabled(true);
                  spinBoxMin->setRange(-128, 127);
                  spinBoxMax->setRange(-128, 127);
                  spinBoxMin->setValue(c->minVal());
                  spinBoxMax->setValue(c->maxVal());
                  enableDefaultControls(true, false);
                  break;
            case MidiController::Controller14:
            case MidiController::RPN14:
            case MidiController::NRPN14:
                  spinBoxHCtrlNo->setEnabled(true);
                  spinBoxLCtrlNo->setEnabled(true);
                  spinBoxHCtrlNo->setValue(ctrlH);
                  spinBoxLCtrlNo->setValue(ctrlL);
                  spinBoxMin->setEnabled(true);
                  spinBoxMax->setEnabled(true);
                  spinBoxMin->setRange(-16384, 16383);
                  spinBoxMax->setRange(-16384, 16383);
                  spinBoxMin->setValue(c->minVal());
                  spinBoxMax->setValue(c->maxVal());
                  enableDefaultControls(true, false);
                  break;
            case MidiController::Pitch:
                  spinBoxHCtrlNo->setEnabled(false);
                  spinBoxLCtrlNo->setEnabled(false);
                  spinBoxHCtrlNo->setValue(0);
                  spinBoxLCtrlNo->setValue(0);
                  spinBoxMin->setEnabled(true);
                  spinBoxMax->setEnabled(true);
                  spinBoxMin->setRange(-8192, 8191);
                  spinBoxMax->setRange(-8192, 8191);
                  spinBoxMin->setValue(c->minVal());
                  spinBoxMax->setValue(c->maxVal());
                  enableDefaultControls(true, false);
                  break;
            case MidiController::Program:
                  spinBoxHCtrlNo->setEnabled(false);
                  spinBoxLCtrlNo->setEnabled(false);
                  spinBoxHCtrlNo->setValue(0);
                  spinBoxLCtrlNo->setValue(0);
                  spinBoxMin->setEnabled(false);
                  spinBoxMax->setEnabled(false);
                  spinBoxMin->setRange(0, 0);
                  spinBoxMax->setRange(0, 0);
                  spinBoxMin->setValue(0);
                  spinBoxMax->setValue(0);
                  enableDefaultControls(false, true);
                  break;
            default:
                  spinBoxHCtrlNo->setEnabled(false);
                  spinBoxLCtrlNo->setEnabled(false);
                  spinBoxMin->setEnabled(false);
                  spinBoxMax->setEnabled(false);
                  enableDefaultControls(false, false);
                  break;
            }      
            
      if(type == MidiController::Program)
      {
        spinBoxDefault->setRange(0, 0);
        spinBoxDefault->setValue(0);
        setDefaultPatchControls(c->initVal());
      }
      else
      {
        spinBoxDefault->setRange(c->minVal() - 1, c->maxVal());
        if(c->initVal() == CTRL_VAL_UNKNOWN)
          //spinBoxDefault->setValue(c->minVal() - 1);
          spinBoxDefault->setValue(spinBoxDefault->minValue());
        else  
          spinBoxDefault->setValue(c->initVal());
      }
      
      //moveWithPart->setChecked(c->moveWithPart());
      
      spinBoxHCtrlNo->blockSignals(false);
      spinBoxLCtrlNo->blockSignals(false);
      spinBoxMin->blockSignals(false);
      spinBoxMax->blockSignals(false);
      spinBoxDefault->blockSignals(false);
      }

//---------------------------------------------------------
//   ctrlNameReturn
//---------------------------------------------------------

void EditInstrument::ctrlNameReturn()
//void EditInstrument::ctrlNameChanged(const QString& s)
{
      ListViewData* item = (ListViewData*)viewController->selectedItem();
      if (item == 0)
            return;
      MidiController* c = (MidiController*)item->data();
      
      QString cName = ctrlName->text();
      
      if(c->name() == cName)
        return;
      
      //MidiControllerList* cl = instrument->controller();
      MidiControllerList* cl = workingInstrument.controller();
      for(ciMidiController ic = cl->begin(); ic != cl->end(); ++ic) 
      {
        MidiController* mc = ic->second;
        if(mc != c && mc->name() == cName) 
        {
          ctrlName->blockSignals(true);
          ctrlName->setText(c->name());
          ctrlName->blockSignals(false);
          
          QMessageBox::critical(this,
              tr("MusE: Bad controller name"),
              tr("Please choose a unique controller name"),
              QMessageBox::Ok,
              Qt::NoButton,
              Qt::NoButton);
              
          return;
        }
      }
      
      c->setName(ctrlName->text());
      item->setText(COL_NAME, ctrlName->text());
      //c->setName(s);
      //item->setText(COL_NAME, s);
      workingInstrument.setDirty(true);
}

//---------------------------------------------------------
//   ctrlTypeChanged
//---------------------------------------------------------

void EditInstrument::ctrlTypeChanged(int idx)
      {
      ListViewData* item = (ListViewData*)viewController->selectedItem();
      if (item == 0)
            return;
      
      MidiController::ControllerType t = (MidiController::ControllerType)idx;
      MidiController* c = (MidiController*)item->data();
      if(t == midiControllerType(c->num()))
         return;
      
      //if(item)
        item->setText(COL_TYPE, ctrlType->currentText());
      
      int hnum = 0, lnum = 0;
      //int rng = 0;
      //int min = 0, max = 0;
      
      spinBoxMin->blockSignals(true);
      spinBoxMax->blockSignals(true);
      spinBoxDefault->blockSignals(true);
      
      switch (t) {
            //case MidiController::RPN:
            //case MidiController::NRPN:
            case MidiController::Controller7:
                  spinBoxHCtrlNo->setEnabled(false);
                  spinBoxLCtrlNo->setEnabled(true);
                  spinBoxMin->setEnabled(true);
                  spinBoxMax->setEnabled(true);
                  enableDefaultControls(true, false);
                  spinBoxMin->setRange(-128, 127);
                  spinBoxMax->setRange(-128, 127);
                  
                  spinBoxMin->setValue(0);
                  spinBoxMax->setValue(127);
                  spinBoxDefault->setRange(spinBoxMin->value() - 1, spinBoxMax->value());
                  
                  spinBoxDefault->setValue(spinBoxDefault->minValue());
                  lnum = spinBoxLCtrlNo->value();
                  //rng = 127;
                  //min = -128;
                  //max = 127;
                  //if(item)
                  //{
                    //item->setText(COL_LNUM, QString().setNum(spinBoxLCtrlNo->value()));
                    if(lnum == -1)
                      item->setText(COL_LNUM, QString("*"));
                    else  
                      item->setText(COL_LNUM, QString().setNum(lnum));
                    item->setText(COL_HNUM, QString("---"));
                    item->setText(COL_MIN, QString().setNum(spinBoxMin->value()));
                    item->setText(COL_MAX, QString().setNum(spinBoxMax->value()));
                    item->setText(COL_DEF, QString("---"));
                  //}  
                  break;
            case MidiController::RPN:
            case MidiController::NRPN:
                  spinBoxHCtrlNo->setEnabled(true);
                  spinBoxLCtrlNo->setEnabled(true);
                  spinBoxMin->setEnabled(true);
                  spinBoxMax->setEnabled(true);
                  enableDefaultControls(true, false);
                  spinBoxMin->setRange(-128, 127);
                  spinBoxMax->setRange(-128, 127);
                  
                  spinBoxMin->setValue(0);
                  spinBoxMax->setValue(127);
                  spinBoxDefault->setRange(spinBoxMin->value() - 1, spinBoxMax->value());
                  spinBoxDefault->setValue(spinBoxDefault->minValue());
                  
                  hnum = spinBoxHCtrlNo->value();
                  lnum = spinBoxLCtrlNo->value();
                  //rng = 127;
                  //min = -128;
                  //max = 127;
                  //if(item)
                  //{
                    //item->setText(COL_LNUM, QString().setNum(spinBoxLCtrlNo->value()));
                    //item->setText(COL_HNUM, QString().setNum(spinBoxHCtrlNo->value()));
                    if(lnum == -1)
                      item->setText(COL_LNUM, QString("*"));
                    else  
                      item->setText(COL_LNUM, QString().setNum(lnum));
                    item->setText(COL_HNUM, QString().setNum(hnum));
                    item->setText(COL_MIN, QString().setNum(spinBoxMin->value()));
                    item->setText(COL_MAX, QString().setNum(spinBoxMax->value()));
                    item->setText(COL_DEF, QString("---"));
                  //}  
                  break;
            case MidiController::Controller14:
            case MidiController::RPN14:
            case MidiController::NRPN14:
                  spinBoxHCtrlNo->setEnabled(true);
                  spinBoxLCtrlNo->setEnabled(true);
                  spinBoxMin->setEnabled(true);
                  spinBoxMax->setEnabled(true);
                  enableDefaultControls(true, false);
                  spinBoxMin->setRange(-16384, 16383);
                  spinBoxMax->setRange(-16384, 16383);
                  
                  spinBoxMin->setValue(0);
                  spinBoxMax->setValue(16383);
                  spinBoxDefault->setRange(spinBoxMin->value() - 1, spinBoxMax->value());
                  spinBoxDefault->setValue(spinBoxDefault->minValue());
                  
                  hnum = spinBoxHCtrlNo->value();
                  lnum = spinBoxLCtrlNo->value();
                  //rng = 16383;
                  //min = -16384;
                  //max = 16383;
                  //if(item)
                  //{
                    //item->setText(COL_LNUM, QString().setNum(spinBoxLCtrlNo->value()));
                    //item->setText(COL_HNUM, QString().setNum(spinBoxHCtrlNo->value()));
                    if(lnum == -1)
                      item->setText(COL_LNUM, QString("*"));
                    else  
                      item->setText(COL_LNUM, QString().setNum(lnum));
                    item->setText(COL_HNUM, QString().setNum(hnum));
                    item->setText(COL_MIN, QString().setNum(spinBoxMin->value()));
                    item->setText(COL_MAX, QString().setNum(spinBoxMax->value()));
                    item->setText(COL_DEF, QString("---"));
                  //}  
                  break;
            case MidiController::Pitch:
                  spinBoxHCtrlNo->setEnabled(false);
                  spinBoxLCtrlNo->setEnabled(false);
                  spinBoxMin->setEnabled(true);
                  spinBoxMax->setEnabled(true);
                  enableDefaultControls(true, false);
                  spinBoxMin->setRange(-8192, 8191);
                  spinBoxMax->setRange(-8192, 8191);
                  
                  spinBoxMin->setValue(-8192);
                  spinBoxMax->setValue(8191);
                  spinBoxDefault->setRange(spinBoxMin->value() - 1, spinBoxMax->value());
                  spinBoxDefault->setValue(spinBoxDefault->minValue());
                  
                  //rng = 8191;
                  //min = -8192;
                  //max = 8191;
                  //if(item)
                  //{
                    item->setText(COL_LNUM, QString("---"));
                    item->setText(COL_HNUM, QString("---"));
                    item->setText(COL_MIN, QString().setNum(spinBoxMin->value()));
                    item->setText(COL_MAX, QString().setNum(spinBoxMax->value()));
                    item->setText(COL_DEF, QString("---"));
                  //}  
                  break;
            case MidiController::Program:
                  spinBoxHCtrlNo->setEnabled(false);
                  spinBoxLCtrlNo->setEnabled(false);
                  spinBoxMin->setEnabled(false);
                  spinBoxMax->setEnabled(false);
                  enableDefaultControls(false, true);
                  spinBoxMin->setRange(0, 0);
                  spinBoxMax->setRange(0, 0);
                  
                  spinBoxMin->setValue(0);
                  spinBoxMax->setValue(0);
                  spinBoxDefault->setRange(0, 0);
                  spinBoxDefault->setValue(0);
                  
                  //if(item)
                  //{
                    item->setText(COL_LNUM, QString("---"));
                    item->setText(COL_HNUM, QString("---"));
                    item->setText(COL_MIN, QString("---"));
                    item->setText(COL_MAX, QString("---"));
                    
                    //item->setText(COL_DEF, QString("0-0-0"));
                    item->setText(COL_DEF, QString("---"));
                  //}  
                  break;
            // Shouldn't happen...
            default:
                  spinBoxHCtrlNo->setEnabled(false);
                  spinBoxLCtrlNo->setEnabled(false);
                  spinBoxMin->setEnabled(false);
                  spinBoxMax->setEnabled(false);
                  enableDefaultControls(false, false);
                  
                  spinBoxMin->blockSignals(false);
                  spinBoxMax->blockSignals(false);
                  return;
                  
                  break;
            }      
            
      spinBoxMin->blockSignals(false);
      spinBoxMax->blockSignals(false);
      spinBoxDefault->blockSignals(false);
      
      c->setNum(MidiController::genNum(t, hnum, lnum));
      
      setDefaultPatchControls(0xffffff);
      if(t == MidiController::Program)
      {
        c->setMinVal(0);
        c->setMaxVal(0xffffff);
        c->setInitVal(0xffffff);
      }
      else
      {
        c->setMinVal(spinBoxMin->value());
        c->setMaxVal(spinBoxMax->value());
        if(spinBoxDefault->value() == spinBoxDefault->minValue())
          c->setInitVal(CTRL_VAL_UNKNOWN);
        else  
          c->setInitVal(spinBoxDefault->value());
      }  
      
      
      /*
      
      if(rng != 0)
      {
        int mn = c->minVal();
        int mx = c->maxVal();
        //if(val > item->text(COL_MAX).toInt())
        if(mx > max)
        {
          c->setMaxVal(max);
          spinBoxMax->blockSignals(true);
          spinBoxMax->setValue(max);
          spinBoxMax->blockSignals(false);
          if(item)
            item->setText(COL_MAX, QString().setNum(max));
        }  
        //else
        if(mn < min)
        {
          c->setMinVal(min);
          spinBoxMin->blockSignals(true);
          spinBoxMin->setValue(min);
          spinBoxMin->blockSignals(false);
          if(item)
            item->setText(COL_MIN, QString().setNum(min));
        }  
        //else
        if(mx - mn > rng)
        {
          //mx = val + rng;
          c->setMinVal(0);
          c->setMaxVal(rng);
          spinBoxMin->blockSignals(true);
          spinBoxMax->blockSignals(true);
          spinBoxMin->setValue(0);
          spinBoxMax->setValue(rng);
          spinBoxMin->blockSignals(false);
          spinBoxMax->blockSignals(false);
          if(item)
          {
            item->setText(COL_MIN, QString().setNum(0));
            item->setText(COL_MAX, QString().setNum(rng));
          }  
        }  
        
        spinBoxDefault->blockSignals(true);
        
        spinBoxDefault->setRange(spinBoxMin->value() - 1, spinBoxMax->value());
        int inval = c->initVal();
        if(inval == CTRL_VAL_UNKNOWN)
          spinBoxDefault->setValue(spinBoxDefault->minValue());
        else  
        {
          if(inval < c->minVal())
          {
            c->setInitVal(c->minVal());
            spinBoxDefault->setValue(c->minVal());
          }
          else  
          if(inval > c->maxVal())
          {  
            c->setInitVal(c->maxVal());
            spinBoxDefault->setValue(c->maxVal());
          }  
        } 
         
        //spinBoxDefault->setRange(c->minVal() - 1, c->maxVal());
        spinBoxDefault->blockSignals(false);
        
      }
        
      */
      
      workingInstrument.setDirty(true);
      }

//---------------------------------------------------------
//   ctrlHNumChanged
//---------------------------------------------------------

void EditInstrument::ctrlHNumChanged(int val)
      {
      ListViewData* item = (ListViewData*)viewController->selectedItem();
      if (item == 0)
            return;
      QString s;
      s.setNum(val);
      MidiController* c = (MidiController*)item->data();
      //int n = c->num() & 0xff;
      int n = c->num() & 0x7fff00ff;
      c->setNum(n | ((val & 0xff) << 8));
      item->setText(COL_HNUM, s);
      workingInstrument.setDirty(true);
      }

//---------------------------------------------------------
//   ctrlLNumChanged
//---------------------------------------------------------

void EditInstrument::ctrlLNumChanged(int val)
      {
      ListViewData* item = (ListViewData*)viewController->selectedItem();
      if (item == 0)
            return;
      MidiController* c = (MidiController*)item->data();
      //int n = c->num() & 0xff00;
      int n = c->num() & ~0xff;
      c->setNum(n | (val & 0xff));
      if(val == -1)
        item->setText(COL_LNUM, QString("*"));
      else  
      {
        QString s;
        s.setNum(val);
        item->setText(COL_LNUM, s);
      }  
      workingInstrument.setDirty(true);
      }

//---------------------------------------------------------
//   ctrlMinChanged
//---------------------------------------------------------

void EditInstrument::ctrlMinChanged(int val)
{
      ListViewData* item = (ListViewData*)viewController->selectedItem();
      if (item == 0)
            return;
        
      QString s;
      s.setNum(val);
      item->setText(COL_MIN, s);
      
      MidiController* c = (MidiController*)item->data();
      c->setMinVal(val);
      
      int rng = 0;
      //switch((MidiController::ControllerType)ctrlType->currentItem())
      switch(midiControllerType(c->num()))
      {
            case MidiController::Controller7:
            case MidiController::RPN:
            case MidiController::NRPN:
                  rng = 127;
                  break;
            case MidiController::Controller14:
            case MidiController::RPN14:
            case MidiController::NRPN14:
            case MidiController::Pitch:
                  rng = 16383;
                  break;
            default: 
                  break;      
      }
      
      int mx = c->maxVal();
      
      //if(val > item->text(COL_MAX).toInt())
      if(val > mx)
      {
        c->setMaxVal(val);
        spinBoxMax->blockSignals(true);
        spinBoxMax->setValue(val);
        spinBoxMax->blockSignals(false);
        item->setText(COL_MAX, s);
      }  
      else
      if(mx - val > rng)
      {
        mx = val + rng;
        c->setMaxVal(mx);
        spinBoxMax->blockSignals(true);
        spinBoxMax->setValue(mx);
        spinBoxMax->blockSignals(false);
        item->setText(COL_MAX, QString().setNum(mx));
      }  
      
      spinBoxDefault->blockSignals(true);
      
      spinBoxDefault->setRange(spinBoxMin->value() - 1, spinBoxMax->value());
      
      int inval = c->initVal();
      if(inval == CTRL_VAL_UNKNOWN)
        spinBoxDefault->setValue(spinBoxDefault->minValue());
      else  
      {
        if(inval < c->minVal())
        {
          c->setInitVal(c->minVal());
          spinBoxDefault->setValue(c->minVal());
        }
        else  
        if(inval > c->maxVal())
        {  
          c->setInitVal(c->maxVal());
          spinBoxDefault->setValue(c->maxVal());
        }  
      } 
      
      spinBoxDefault->blockSignals(false);
      
      workingInstrument.setDirty(true);
}

//---------------------------------------------------------
//   ctrlMaxChanged
//---------------------------------------------------------

void EditInstrument::ctrlMaxChanged(int val)
{
      ListViewData* item = (ListViewData*)viewController->selectedItem();
      if (item == 0)
            return;
        
      QString s;
      s.setNum(val);
      item->setText(COL_MAX, s);
      
      MidiController* c = (MidiController*)item->data();
      c->setMaxVal(val);
      
      int rng = 0;
      //switch((MidiController::ControllerType)ctrlType->currentItem())
      switch(midiControllerType(c->num()))
      {
            case MidiController::Controller7:
            case MidiController::RPN:
            case MidiController::NRPN:
                  rng = 127;
                  break;
            case MidiController::Controller14:
            case MidiController::RPN14:
            case MidiController::NRPN14:
            case MidiController::Pitch:
                  rng = 16383;
                  break;
            default: 
                  break;      
      }
      
      int mn = c->minVal();
      
      //if(val < item->text(COL_MIN).toInt())
      if(val < mn)
      {
        c->setMinVal(val);
        spinBoxMin->blockSignals(true);
        spinBoxMin->setValue(val);
        spinBoxMin->blockSignals(false);
        item->setText(COL_MIN, s);
      }  
      else
      if(val - mn > rng)
      {
        mn = val - rng;
        c->setMinVal(mn);
        spinBoxMin->blockSignals(true);
        spinBoxMin->setValue(mn);
        spinBoxMin->blockSignals(false);
        item->setText(COL_MIN, QString().setNum(mn));
      }  
      
      spinBoxDefault->blockSignals(true);
      
      spinBoxDefault->setRange(spinBoxMin->value() - 1, spinBoxMax->value());
      
      int inval = c->initVal();
      if(inval == CTRL_VAL_UNKNOWN)
        spinBoxDefault->setValue(spinBoxDefault->minValue());
      else  
      {
        if(inval < c->minVal())
        {
          c->setInitVal(c->minVal());
          spinBoxDefault->setValue(c->minVal());
        }
        else  
        if(inval > c->maxVal())
        {  
          c->setInitVal(c->maxVal());
          spinBoxDefault->setValue(c->maxVal());
        }  
      } 
      
      spinBoxDefault->blockSignals(false);
      
      workingInstrument.setDirty(true);
}

//---------------------------------------------------------
//   ctrlDefaultChanged
//---------------------------------------------------------

void EditInstrument::ctrlDefaultChanged(int val)
{
      ListViewData* item = (ListViewData*)viewController->selectedItem();
      if (item == 0)
            return;
        
      MidiController* c = (MidiController*)item->data();
      
      if(val == c->minVal() - 1)
      {
        c->setInitVal(CTRL_VAL_UNKNOWN);
        item->setText(COL_DEF, QString("---"));
      }  
      else
      {
        c->setInitVal(val);
        item->setText(COL_DEF, QString().setNum(val));
      }
      workingInstrument.setDirty(true);
}

//---------------------------------------------------------
//   ctrlNullParamHChanged
//---------------------------------------------------------

void EditInstrument::ctrlNullParamHChanged(int nvh)
{
  int nvl = nullParamSpinBoxL->value();
  if(nvh == -1)
  {
    nullParamSpinBoxL->blockSignals(true);
    nullParamSpinBoxL->setValue(-1);
    nullParamSpinBoxL->blockSignals(false);
    nvl = -1;
  }
  else
  {
    if(nvl == -1)
    {
      nullParamSpinBoxL->blockSignals(true);
      nullParamSpinBoxL->setValue(0);
      nullParamSpinBoxL->blockSignals(false);
      nvl = 0;
    }
  }
  if(nvh == -1 && nvl == -1)
    workingInstrument.setNullSendValue(-1);
  else  
    workingInstrument.setNullSendValue((nvh << 8) | nvl);
  workingInstrument.setDirty(true);
}

//---------------------------------------------------------
//   ctrlNullParamLChanged
//---------------------------------------------------------

void EditInstrument::ctrlNullParamLChanged(int nvl)
{
  int nvh = nullParamSpinBoxH->value();
  if(nvl == -1)
  {
    nullParamSpinBoxH->blockSignals(true);
    nullParamSpinBoxH->setValue(-1);
    nullParamSpinBoxH->blockSignals(false);
    nvh = -1;
  }
  else
  {
    if(nvh == -1)
    {
      nullParamSpinBoxH->blockSignals(true);
      nullParamSpinBoxH->setValue(0);
      nullParamSpinBoxH->blockSignals(false);
      nvh = 0;
    }
  }
  if(nvh == -1 && nvl == -1)
    workingInstrument.setNullSendValue(-1);
  else  
    workingInstrument.setNullSendValue((nvh << 8) | nvl);
  workingInstrument.setDirty(true);
}

//---------------------------------------------------------
//   deletePatchClicked
//---------------------------------------------------------

void EditInstrument::deletePatchClicked()
      {
      //QListWidgetItem* item = instrumentList->currentItem();
//      ListBoxData* item = (ListBoxData*)instrumentList->selectedItem();
//      if (item == 0)
//            return;
      //MidiInstrument* instrument = (MidiInstrument*)item->data(Qt::UserRole).value<void*>();
//      MidiInstrument* instrument = (MidiInstrument*)item->data();
      //QTreeWidgetItem* pi = patchView->currentItem();
      ListViewData* pi = (ListViewData*)patchView->selectedItem();
      if (pi == 0)
            return;
      
      //void* p = pi->data(0, Qt::UserRole).value<void*>();
//      Patch* patch = (Patch*)pi->data();
      //if (p == 0)
      // If patch is zero it's a patch group.
//      if(patch == 0)
      
      // If the item has a parent item, it's a patch item...
      if(pi->parent())
      {
        PatchGroup* group = (PatchGroup*)((ListViewData*)pi->parent())->data();
        
        // If there is an allocated patch in the data, delete it.
        //Patch* patch = (Patch*)pi->auxData();
        Patch* patch = (Patch*)pi->data();
        if(patch)
        {
          if(group)
          {
            //for(iPatch ip = group->patches.begin(); ip != group->patches.end(); ++ip)
            //  if(&*ip == patch)
            //  if(*ip == patch)
            //  {
            //    
            //    printf("deletePatchClicked: erasing patch\n");
            //    
            //    group->patches.erase(ip);
            //    break;
            //  }      
            //group->patches.remove( (const Patch&)(*patch) );
            group->patches.remove(patch);
          }  
          delete patch;
        }  
      }
      else
      // The item has no parent item, it's a patch group item...
      {
        // Is there an allocated patch group in the data?
        //PatchGroup* group = (PatchGroup*)pi->auxData();
        PatchGroup* group = (PatchGroup*)pi->data();
        if(group)
        {
          
          PatchGroupList* pg = workingInstrument.groups();
          //for(ciPatchGroup ipg = pg->begin(); ipg != pg->end(); ++ipg)
          for(iPatchGroup ipg = pg->begin(); ipg != pg->end(); ++ipg)
          {
            
            //printf("deletePatchClicked: working patch group name:%s ad:%X group name:%s ad:%X\n", (*ipg)->name.latin1(), (unsigned int)(*ipg), group->name.latin1(), (unsigned int) group);
            
            //if(&*ipg == group)
            if(*ipg == group)
            {
              pg->erase(ipg);
              break;
            }  
          }
          
          // Iterate all child list view (patch) items. Find and delete any allocated patches in the items' data.
//          for(ListViewData* i = (ListViewData*)pi->firstChild(); i; i = (ListViewData*)i->nextSibling()) 
//          {
            //Patch* patch = (Patch*)i->auxData();
//            Patch* patch = (Patch*)i->data();
//            if(patch)
//            {
              //delete patch;
              //group->patches.remove(*patch);
              const PatchList& pl = group->patches;
              for(ciPatch ip = pl.begin(); ip != pl.end(); ++ip)
              {
//                if(&*ip == patch)
//                {
//                  group->patches.erase(ip);
//                  break;
//                }

                // Delete the patch.
                if(*ip)
                  delete *ip;  
              }
              
              //group->patches.clear();  
              
//            }  
//          }
          
          // Now delete the group.
          delete group;
          
        }  
      }
      
      //oldPatchItem = (ListViewData*)patchView->selectedItem();
      //oldPatchItem = 0;
      
      // Now delete the patch or group item (and any child patch items) from the list view tree.
      // !!! This will trigger a patchChanged call. 
      patchView->blockSignals(true);
      delete pi;
      if(patchView->currentItem())
        patchView->setSelected(patchView->currentItem(), true);
      patchView->blockSignals(false);
      
      oldPatchItem = 0;
      patchChanged();
      
      //Patch* patch = (Patch*)p;
      
      //std::vector<PatchGroup>* pg = instrument->groups();
      //for (std::vector<PatchGroup>::iterator g = pg->begin(); g != pg->end(); ++g) {
      //      for (iPatch p = g->patches.begin(); p != g->patches.end(); ++p) {
      //            if (patch == *p) {
      //                  g->patches.erase(p);
      //                  delete pi;
      //                  instrument->setDirty(true);
      //                  return;
      //                  }
      //            }
      //      }
      //printf("fatal: patch not found\n");      
      //delete patch;
      //delete pi;
      
      
      workingInstrument.setDirty(true);
      }

//---------------------------------------------------------
//   newPatchClicked
//---------------------------------------------------------

void EditInstrument::newPatchClicked()
      {
      //QListWidgetItem* item = instrumentList->currentItem();
//      ListBoxData* item = (ListBoxData*)instrumentList->selectedItem();
//      if (item == 0)
//            return;
      
      if(oldPatchItem)
      {
        if(oldPatchItem->parent())
          updatePatch(&workingInstrument, (Patch*)oldPatchItem->data());
        else  
          updatePatchGroup(&workingInstrument, (PatchGroup*)oldPatchItem->data());
      }  
      
      //MidiInstrument* instrument = (MidiInstrument*)item->data(Qt::UserRole).value<void*>();
//      MidiInstrument* instrument = (MidiInstrument*)item->data();
      //std::vector<PatchGroup>* pg = instrument->groups();
//      PatchGroupList* pg = instrument->groups();
      PatchGroupList* pg = workingInstrument.groups();
      QString patchName;
      for (int i = 1;; ++i) {
            patchName = QString("Patch-%1").arg(i);
            bool found = false;

            //for (std::vector<PatchGroup>::iterator g = pg->begin(); g != pg->end(); ++g) {
            for (iPatchGroup g = pg->begin(); g != pg->end(); ++g) {
                  PatchGroup* pgp = *g;
                  //for (iPatch p = g->patches.begin(); p != g->patches.end(); ++p) {
                  for (iPatch p = pgp->patches.begin(); p != pgp->patches.end(); ++p) {
                        //if (p->name == patchName) {
                        if ((*p)->name == patchName) {
                              found = true;
                              break;
                              }
                        }
                  if (found)
                        break;
                  }
            if (!found)
                  break;
            }

      //
      // search current patch group
      //
      //PatchGroup* pGroup = 0;
      //QTreeWidgetItem* pi = patchView->currentItem();
      ListViewData* pi = (ListViewData*)patchView->selectedItem();
      if (pi == 0)
            return;
      
      // If there is data then pi is a patch item, and there must be a parent patch group item (with null data).
      //if (pi->data(0, Qt::UserRole).value<void*>())
      //if (pi->data())
      
      Patch* selpatch = 0;
      
      // If there is a parent item then pi is a patch item, and there must be a parent patch group item.
      if(pi->parent())
      {
        // Remember the current selected patch.
        selpatch = (Patch*)pi->data();
        
        pi = (ListViewData*)pi->parent();
      }
      
      PatchGroup* group = (PatchGroup*)pi->data();
      if(!group)
        return;
        
      //for (std::vector<PatchGroup>::iterator g = pg->begin(); g != pg->end(); ++g) {
//      for (ciPatchGroup g = pg->begin(); g != pg->end(); ++g) {
//            if (g->name == pi->text(0)) {
//                  pGroup = &*g;
//                  break;
//                  }            
//            }
//      if (pGroup == 0) {
//            printf("group not found\n");
//            return;
//            }
      
      // Create a new Patch, then store its pointer in a new patch item, 
      //  to be added later to the patch group only upon save...
      //Patch patch;
      //patch.name = patchName;
      Patch* patch = new Patch;
      int hb  = -1;
      int lb  = -1;
      int prg = 0;
      patch->hbank = hb;
      patch->lbank = lb;
      patch->prog = prg;
      patch->typ = -1;                     
      patch->drum = false;
      
      if(selpatch)
      {
        hb  = selpatch->hbank;
        lb  = selpatch->lbank;
        prg = selpatch->prog;
        patch->typ = selpatch->typ;                     
        patch->drum = selpatch->drum;                     
      }
      
      bool found = false;
      
      // The 129 is to accommodate -1 values. Yes, it may cause one extra redundant loop but hey, 
      //  if it hasn't found an available patch number by then, another loop won't matter.
      for(int k = 0; k < 129; ++k)
      {
        for(int j = 0; j < 129; ++j)
        {
          for(int i = 0; i < 128; ++i) 
          {
            found = false;

            for(iPatchGroup g = pg->begin(); g != pg->end(); ++g) 
            {
              PatchGroup* pgp = *g;
              for(iPatch ip = pgp->patches.begin(); ip != pgp->patches.end(); ++ip) 
              {
                Patch* p = *ip;
                if((p->prog  == ((prg + i) & 0x7f)) && 
                   ((p->lbank == -1 && lb == -1) || (p->lbank == ((lb + j) & 0x7f))) && 
                   ((p->hbank == -1 && hb == -1) || (p->hbank == ((hb + k) & 0x7f)))) 
                {
                  found = true;
                  break;
                }
              }
              if(found)
                break;
            }
            
            if(!found)
            {
              patch->prog  = (prg + i) & 0x7f;
              if(lb == -1)
                patch->lbank = -1;
              else  
                patch->lbank = (lb + j) & 0x7f;
                
              if(hb == -1)
                patch->hbank = -1;
              else    
                patch->hbank = (hb + k) & 0x7f;
                
              //patch->typ = selpatch->typ;                     
              //patch->drum = selpatch->drum;                     
              break;
            } 
              
          }
          if(!found)
            break;
        }  
        if(!found)
          break;
      }  
      
      patch->name = patchName;

      group->patches.push_back(patch);
      //Patch* pp = &(group->patches.back());
      
      //QTreeWidgetItem* sitem = new QTreeWidgetItem;
      ListViewData* sitem = new ListViewData(pi);
      //sitem->setText(0, patch->name);
      sitem->setText(0, patchName);
      
      patchNameEdit->setText(patchName);
      
      //QVariant v = QVariant::fromValue((void*)(patch));
      //sitem->setData(0, Qt::UserRole, v);
      
      // Set the list view item's data. 
      sitem->setData((void*)patch);
      //sitem->setAuxData((void*)patch);
      //sitem->setData((void*)pp);

      //pi->addChild(sitem);
      
      //printf("newPatchClicked: before patchView->setCurrentItem\n");
      
      //patchView->setCurrentItem(sitem);
      
      //printf("newPatchClicked: after patchView->setCurrentItem\n");
      
      //oldPatchItem = 0;
      
      // May cause patchChanged call.
      patchView->blockSignals(true);
      patchView->setSelected(sitem, true);
      patchView->ensureItemVisible(sitem);
      patchView->blockSignals(false);
      
      //oldPatchItem = (ListViewData*)patchView->selectedItem();
      //oldPatchItem = sitem;
      //oldPatchItem = 0;
      
      spinBoxHBank->setEnabled(true);
      spinBoxLBank->setEnabled(true);
      spinBoxProgram->setEnabled(true);
      checkBoxDrum->setEnabled(true);
      checkBoxGM->setEnabled(true);
      checkBoxGS->setEnabled(true);
      checkBoxXG->setEnabled(true);
      
      oldPatchItem = 0;
      patchChanged();
      
      //instrument->setDirty(true);
      workingInstrument.setDirty(true);
      }

//---------------------------------------------------------
//   newGroupClicked
//---------------------------------------------------------

void EditInstrument::newGroupClicked()
      {
      //QListWidgetItem* item = instrumentList->currentItem();
//      ListBoxData* item = (ListBoxData*)instrumentList->selectedItem();
//      if (item == 0)
//            return;
      
      if(oldPatchItem)
      {
        if(oldPatchItem->parent())
          updatePatch(&workingInstrument, (Patch*)oldPatchItem->data());
        else  
          updatePatchGroup(&workingInstrument, (PatchGroup*)oldPatchItem->data());
      }  
      
      //MidiInstrument* instrument = (MidiInstrument*)item->data(Qt::UserRole).value<void*>();
//      MidiInstrument* instrument = (MidiInstrument*)item->data();
      //std::vector<PatchGroup>* pg = instrument->groups();
//      PatchGroupList* pg = instrument->groups();
      PatchGroupList* pg = workingInstrument.groups();
      QString groupName;
      for (int i = 1;; ++i) {
            groupName = QString("Group-%1").arg(i);
            bool found = false;

            //for (std::vector<PatchGroup>::iterator g = pg->begin(); g != pg->end(); ++g) {
            for (ciPatchGroup g = pg->begin(); g != pg->end(); ++g) {
                  //if (g->name == groupName) {
                  if ((*g)->name == groupName) {
                        found = true;
                        break;
                        }
                  }
            if (!found)
                  break;
            }

      // Create a new PatchGroup, then store its pointer in a new patch group item, 
      //  to be added later to the instrument only upon save...
      PatchGroup* group = new PatchGroup;
      group->name = groupName;
      //PatchGroup group;
      //group.name = groupName;
      
      pg->push_back(group);
      //PatchGroup* pgp = &(pg->back());
      
      //QTreeWidgetItem* sitem = new QTreeWidgetItem;
      ListViewData* sitem = new ListViewData(patchView);
      sitem->setText(0, groupName);
      
      patchNameEdit->setText(groupName);
      
      //QVariant v = QVariant::fromValue((void*)0);
      //sitem->setData(0, Qt::UserRole, v);
      //sitem->setData((void*)0);
      
      // Set the list view item's data. 
      sitem->setData((void*)group);
      //sitem->setAuxData((void*)pgp);
      
      //patchView->addTopLevelItem(sitem);
      //patchView->setCurrentItem(sitem);
      
      //oldPatchItem = 0;
      
      // May cause patchChanged call.
      patchView->blockSignals(true);
      patchView->setSelected(sitem, true);
      patchView->blockSignals(false);
      
      //oldPatchItem = (ListViewData*)patchView->selectedItem();
      oldPatchItem = sitem;
      //oldPatchItem = 0;
      //patchChanged();
      
      spinBoxHBank->setEnabled(false);
      spinBoxLBank->setEnabled(false);
      spinBoxProgram->setEnabled(false);
      checkBoxDrum->setEnabled(false);
      checkBoxGM->setEnabled(false);
      checkBoxGS->setEnabled(false);
      checkBoxXG->setEnabled(false);
      
      //instrument->setDirty(true);
      workingInstrument.setDirty(true);
      }

//---------------------------------------------------------
//   deleteControllerClicked
//---------------------------------------------------------

void EditInstrument::deleteControllerClicked()
      {
      //QListWidgetItem* item = instrumentList->currentItem();
      //ListBoxData* item = (ListBoxData*)instrumentList->selectedItem();
      //QListWidgetItem* item2 = listController->currentItem();
//      ListBoxData* item = (ListBoxData*)listController->selectedItem();
      ListViewData* item = (ListViewData*)viewController->selectedItem();
      
      //if (item == 0 || item2 == 0)
      if(!item)
        return;
      //MidiInstrument* instrument = (MidiInstrument*)item->data(Qt::UserRole).value<void*>();
//      MidiInstrument* instrument = (MidiInstrument*)item->data();
      //MidiController* ctrl       = (MidiController*)item2->data(Qt::UserRole).value<void*>();
      //MidiController* ctrl       = (MidiController*)item2->data();
      //MidiControllerList* cl     = instrument->controller();
      //cl->removeAll(ctrl);
      
      MidiController* ctrl = (MidiController*)item->data();
      if(!ctrl)
        return;
        
      workingInstrument.controller()->erase(ctrl->num());   
      // Now delete the controller.
      delete ctrl;
      
      // Now remove the controller item from the list.
      // This may cause a controllerChanged call.
//      listController->blockSignals(true);
      viewController->blockSignals(true);
      delete item;
      if(viewController->currentItem())
        viewController->setSelected(viewController->currentItem(), true);
//      listController->blockSignals(false);
      viewController->blockSignals(false);
      
      //oldController = (ListBoxData*)listController->selectedItem();
//      oldController = 0;
      
      controllerChanged();
      
      //instrument->setDirty(true);
      workingInstrument.setDirty(true);
      }

//---------------------------------------------------------
//   newControllerClicked
//---------------------------------------------------------

void EditInstrument::newControllerClicked()
      {
      //QListWidgetItem* item = instrumentList->currentItem();
      //if (item == 0)
      //      return;
      //MidiInstrument* instrument = (MidiInstrument*)item->data(Qt::UserRole).value<void*>();

//      if(oldController)
//        updateController(&workingInstrument, (MidiController*)oldController->data());
      
      QString cName;
      //MidiControllerList* cl = instrument->controller();
      MidiControllerList* cl = workingInstrument.controller();
      for (int i = 1;; ++i) {
            //ctrlName = QString("Controller-%d").arg(i);
            cName = QString("Controller-%1").arg(i);
            bool found = false;
            for (iMidiController ic = cl->begin(); ic != cl->end(); ++ic) {
                  MidiController* c = ic->second;
                  if (c->name() == cName) {
                        found = true;
                        break;
                        }
                  }
            if (!found)
              break;
          }

      MidiController* ctrl = new MidiController();
      ctrl->setNum(CTRL_MODULATION);
      ctrl->setMinVal(0);
      ctrl->setMaxVal(127);
      ctrl->setInitVal(CTRL_VAL_UNKNOWN);
      
      ListViewData* ci = (ListViewData*)viewController->selectedItem();
      
      // To allow for quick multiple successive controller creation.
      // If there's a current controller item selected, copy initial values from it.
      bool found = false;
      if(ci)
      {
        MidiController* selctl = (MidiController*)ci->data();
        // Assign.
        // *ctrl = *selctl;
        
        // Auto increment controller number.
        //int l = ctrl->num() & 0x7f;
        //int h = ctrl->num() & 0xffffff00;
        int l = selctl->num() & 0x7f;
        int h = selctl->num() & 0xffffff00;
          
        // Ignore internal controllers and wild cards.
        //if(((h & 0xff0000) != 0x40000) && ((ctrl->num() & 0xff) != 0xff))
        if(((h & 0xff0000) != 0x40000) && ((selctl->num() & 0xff) != 0xff))
        {
          // Assign.
          *ctrl = *selctl;
          
          for (int i = 1; i < 128; ++i) 
          {
            //ctrlName = QString("Controller-%d").arg(i);
            //cName = QString("Controller-%1").arg(i);
            int j = ((i + l) & 0x7f) | h;
            found = false;
            for (iMidiController ic = cl->begin(); ic != cl->end(); ++ic) 
            {
              MidiController* c = ic->second;
              if(c->num() == j) 
              {
                found = true;
                break;
              }
            }
            if(!found)
            {
              ctrl->setNum(j);
              break;
            }  
          }  
        }
      }  
      
      ctrl->setName(cName);
      
      //item = new QListWidgetItem(ctrlName);
//      ListBoxData* item = new ListBoxData(ctrlName);
      
      //QVariant v = qVariantFromValue((void*)(ctrl));
      //item->setData(Qt::UserRole, v);
//      item->setData((void*)ctrl);
      //listController->addItem(item);
//      listController->insertItem(item);
      //listController->setCurrentItem(item);
      
      workingInstrument.controller()->add(ctrl);   
      ListViewData* item = addControllerToView(ctrl);
      
//      listController->blockSignals(true);
//      listController->setSelected(item, true);
//      listController->blockSignals(false);
      viewController->blockSignals(true);
      viewController->setSelected(item, true);
      viewController->blockSignals(false);
      
      //oldController = (ListBoxData*)listController->selectedItem();
//      oldController = item;
      //oldController = 0;
      // MidiController is a class, with itialized values. We have to call this to show the values.
      // To make multiple entries easier, don't bother calling this.
      controllerChanged();
      
      //instrument->setDirty(true);
      workingInstrument.setDirty(true);
      }

//---------------------------------------------------------
//   addControllerClicked
//---------------------------------------------------------

void EditInstrument::addControllerClicked()
{
  //int lnum = listController->currentItem();
  //if(lnum == -1)
  //  return;
    
  //QString name = midiCtrlName(lnum);
  
  int idx = listController->currentItem();
  if(idx == -1)
    return;
  
  int lnum = -1;
  QString name = listController->currentText();
  for(int i = 0; i < 128; i++)
  {
    if(midiCtrlName(i) == name)
    {
      lnum = i;
      break;
    }  
  }
  if(lnum == -1)
  {
    printf("Add controller: Controller not found: %s\n", name.latin1());
    return;
  }
  
  int num = MidiController::genNum(MidiController::Controller7, 0, lnum);
    
  MidiControllerList* cl = workingInstrument.controller();
  for(iMidiController ic = cl->begin(); ic != cl->end(); ++ic) 
  {
    MidiController* c = ic->second;
    if(c->name() == name)
    {
      QMessageBox::critical(this,
          tr("MusE: Cannot add common controller"),
          tr("A controller named ") + name + tr(" already exists."),
          QMessageBox::Ok,
          Qt::NoButton,
          Qt::NoButton);
          
      return;      
    }
    
    if(c->num() == num)
    {
      QMessageBox::critical(this,
          tr("MusE: Cannot add common controller"),
          tr("A controller number ") + QString().setNum(num) + tr(" already exists."),
          QMessageBox::Ok,
          Qt::NoButton,
          Qt::NoButton);
          
      return;      
    }
  }
  
  MidiController* ctrl = new MidiController();
  ctrl->setNum(num);
  ctrl->setMinVal(0);
  ctrl->setMaxVal(127);
  ctrl->setInitVal(CTRL_VAL_UNKNOWN);
  ctrl->setName(name);
  
  workingInstrument.controller()->add(ctrl);   
  
  ListViewData* item = addControllerToView(ctrl);
  
  viewController->blockSignals(true);
  viewController->setSelected(item, true);
  viewController->blockSignals(false);
  
  controllerChanged();
  
  workingInstrument.setDirty(true);
}

/*
//---------------------------------------------------------
//   deleteSysexClicked
//---------------------------------------------------------

void EditInstrument::deleteSysexClicked()
      {
      //QListWidgetItem* item = instrumentList->currentItem();
      //QListWidgetItem* item2 = sysexList->currentItem();
      //if (item == 0 || item2 == 0)
      //      return;
      
      //MidiInstrument* instrument = (MidiInstrument*)item->data(Qt::UserRole).value<void*>();
      //SysEx* sysex  = (SysEx*)item2->data(Qt::UserRole).value<void*>();
      //QList<SysEx*> sl = instrument->sysex();
      //instrument->removeSysex(sysex);
      //delete item2;
      //instrument->setDirty(true);
      
      
      
      ListBoxData* item = (ListBoxData*)sysexList->selectedItem();
      if(!item)
        return;
      
      EventList* el = (EventList*)item->data();
      if(!el)
        return;
        
      }
*/

//---------------------------------------------------------
//   updatePatchGroup
//---------------------------------------------------------

void EditInstrument::updatePatchGroup(MidiInstrument* instrument, PatchGroup* pg)
      {
      if (pg->name != patchNameEdit->text()) {
            pg->name = patchNameEdit->text();
            instrument->setDirty(true);
            }
      }

//---------------------------------------------------------
//   updatePatch
//---------------------------------------------------------

void EditInstrument::updatePatch(MidiInstrument* instrument, Patch* p)
      {
      if (p->name != patchNameEdit->text()) {
            p->name = patchNameEdit->text();
            instrument->setDirty(true);
            }
      
      signed char hb = (spinBoxHBank->value() - 1) & 0xff;
      //if (p->hbank != (spinBoxHBank->value() & 0xff)) {
      //      p->hbank = spinBoxHBank->value() & 0xff;
      if (p->hbank != hb) {
            p->hbank = hb;
            
            instrument->setDirty(true);
            }
      
      signed char lb = (spinBoxLBank->value() - 1) & 0xff;
      //if (p->lbank != (spinBoxLBank->value() & 0xff)) {
      //      p->lbank = spinBoxLBank->value() & 0xff;
      if (p->lbank != lb) {
            p->lbank = lb;
            
            instrument->setDirty(true);
            }
      
      signed char pr = (spinBoxProgram->value() - 1) & 0xff;
      if (p->prog != pr) {
            p->prog = pr;
            
            instrument->setDirty(true);
            }
            
      if (p->drum != checkBoxDrum->isChecked()) {
            p->drum = checkBoxDrum->isChecked();
            instrument->setDirty(true);
            }
      
      // there is no logical xor in c++
      bool a = p->typ & 1;
      bool b = p->typ & 2;
      bool c = p->typ & 4;
      bool aa = checkBoxGM->isChecked();
      bool bb = checkBoxGS->isChecked();
      bool cc = checkBoxXG->isChecked();
      if ((a ^ aa) || (b ^ bb) || (c ^ cc)) {
            int value = 0;
            if (checkBoxGM->isChecked())
                  value |= 1;
            if (checkBoxGS->isChecked())
                  value |= 2;
            if (checkBoxXG->isChecked())
                  value |= 4;
            p->typ = value;
            instrument->setDirty(true);
            }
      
      //if (p->categorie != category->currentIndex()) {
      //      p->categorie = category->currentIndex();
      //      instrument->setDirty(true);
      //      }
      }

/*
//---------------------------------------------------------
//   updateController
//---------------------------------------------------------

void EditInstrument::updateController(MidiInstrument* instrument, MidiController* oc)
      {
      printf("updateController: A\n");
      
      int ctrlH = spinBoxHCtrlNo->value();
      int ctrlL = spinBoxLCtrlNo->value();
      //MidiController::ControllerType type = (MidiController::ControllerType)ctrlType->currentIndex();
      MidiController::ControllerType type = (MidiController::ControllerType)ctrlType->currentItem();
      int num = MidiController::genNum(type, ctrlH, ctrlL);
      //int num = (ctrlH << 8) & 0x7f + ctrlL & 0x7f;

      printf("updateController: B\n");
      
      if (num != oc->num()) {
            
            printf("updateController: num changed, setting dirty. num:%d c->num:%d\n", num, oc->num());
            
            oc->setNum(num);
            instrument->setDirty(true);
            }
            
      if(type != MidiController::Pitch && type != MidiController::Program)
      {
        if (spinBoxMin->value() != oc->minVal()) {
              
              printf("updateController: min changed, setting dirty. min:%d c->min:%d\n", spinBoxMin->value(), oc->minVal());
              
              oc->setMinVal(spinBoxMin->value());
              instrument->setDirty(true);
              }
        if (spinBoxMax->value() != oc->maxVal()) {
              
              printf("updateController: max changed, setting dirty. num:%d max:%d c->max:%d\n", num, spinBoxMax->value(), oc->maxVal());
              
              oc->setMaxVal(spinBoxMax->value());
              instrument->setDirty(true);
              }
        
        int dv = spinBoxDefault->value(); 
        if(dv == oc->minVal() - 1)
          dv = CTRL_VAL_UNKNOWN;
          
        //if (spinBoxDefault->value() != oc->initVal()) {
        if(dv != oc->initVal()) {
              //oc->setInitVal(spinBoxDefault->value());
              oc->setInitVal(dv);
              
              printf("updateController: default changed, setting dirty. def:%d c->init:%d\n", dv, oc->initVal());
              
              instrument->setDirty(true);
              }
      }
      
      
      printf("updateController: C\n");
      
      //if (moveWithPart->isChecked() ^ oc->moveWithPart()) {
      //      oc->setMoveWithPart(moveWithPart->isChecked());
      //      instrument->setDirty(true);
      //      }
      if (ctrlName->text() != oc->name()) {
            oc->setName(ctrlName->text());
            
            printf("updateController: name changed, setting dirty. name:%s c->name:%s\n", ctrlName->text().latin1(), oc->name().latin1());
            
            instrument->setDirty(true);
            }
      //if (ctrlComment->toPlainText() != oc->comment()) {
      //      oc->setComment(ctrlComment->toPlainText());
      //      instrument->setDirty(true);
      //      }
      
      printf("updateController: D\n");
      
      }
*/

//---------------------------------------------------------
//   updateInstrument
//---------------------------------------------------------

void EditInstrument::updateInstrument(MidiInstrument* instrument)
      {
      //QListWidgetItem* sysexItem = sysexList->currentItem();
      //ListBoxData* sysexItem = sysexList->currentItem();
      //if (sysexItem) {
      //      SysEx* so = (SysEx*)sysexItem->data(Qt::UserRole).value<void*>();
      //      updateSysex(instrument, so);
      //      }
      
      //QListWidgetItem* ctrlItem = listController->currentItem();
      //ListBoxData* ctrlItem = (ListBoxData*)listController->currentItem();
      //ListBoxData* ctrlItem = (ListBoxData*)listController->selectedItem();
//      ListViewData* ctrlItem = (ListViewData*)viewController->selectedItem();
      
//      if (ctrlItem) {
            //MidiController* ctrl = (MidiController*)ctrlItem->data(Qt::UserRole).value<void*>();
            
//            printf("updateInstrument: AB\n");
      
//            MidiController* ctrl = (MidiController*)ctrlItem->data();
            
//            printf("updateInstrument: AC\n");
            
//            updateController(instrument, ctrl);
//            }
      
//      printf("updateInstrument: B\n");
      
      //QTreeWidgetItem* patchItem = patchView->currentItem();
      ListViewData* patchItem = (ListViewData*)patchView->selectedItem();
      if (patchItem) 
      {      
        //Patch* p = (Patch*)patchItem->data(0, Qt::UserRole).value<void*>();
        
        // If the item has a parent, it's a patch item.
        if(patchItem->parent())
          updatePatch(instrument, (Patch*)patchItem->data());
        else
          updatePatchGroup(instrument, (PatchGroup*)patchItem->data());
              
      }
    }

//---------------------------------------------------------
//   checkDirty
//    return true on Abort
//---------------------------------------------------------

int EditInstrument::checkDirty(MidiInstrument* i, bool isClose)
      {
      updateInstrument(i);
      if (!i->dirty())
            //return false;
            return 0;
      int n;
      if(isClose) 
        n = QMessageBox::warning(this, tr("MusE"),
         tr("The current Instrument contains unsaved data\n"
         "Save Current Instrument?"),
         tr("&Save"), tr("&Nosave"), tr("&Abort"), 0, 2);
      else   
        n = QMessageBox::warning(this, tr("MusE"),
         tr("The current Instrument contains unsaved data\n"
         "Save Current Instrument?"),
         tr("&Save"), tr("&Nosave"), 0, 1);
      if (n == 0) {
            if (i->filePath().isEmpty())
            {
                  //fileSaveAs();
                  saveAs();
            }      
            else {
                  //QFile f(i->filePath());
                  //if (!f.open(QIODevice::WriteOnly))
                  FILE* f = fopen(i->filePath().latin1(), "w");
                  if(f == 0)
                        //fileSaveAs();
                        saveAs();
                  else {
                        //f.close();
                        if(fclose(f) != 0)
                          printf("EditInstrument::checkDirty: Error closing file\n");
                          
                        if(fileSave(i, i->filePath()))
                              i->setDirty(false);
                        }
                  }
            //return false;
            return 0;
            }
      //return n == 2;
      return n;
      }

//---------------------------------------------------------
//    getPatchItemText
//---------------------------------------------------------

QString EditInstrument::getPatchItemText(int val)
{
  QString s;
  if(val == CTRL_VAL_UNKNOWN)
    s = "---";
  else
  {
    int hb = ((val >> 16) & 0xff) + 1;
    if (hb == 0x100)
      hb = 0;
    int lb = ((val >> 8) & 0xff) + 1;
    if (lb == 0x100)
      lb = 0;
    int pr = (val & 0xff) + 1;
    if (pr == 0x100)
      pr = 0;
    s.sprintf("%d-%d-%d", hb, lb, pr);
  } 
  
  return s;
}                 

//---------------------------------------------------------
//    enableDefaultControls
//---------------------------------------------------------

void EditInstrument::enableDefaultControls(bool enVal, bool enPatch)
{
  spinBoxDefault->setEnabled(enVal);
  patchButton->setEnabled(enPatch);
  if(!enPatch)
  {
    patchButton->blockSignals(true);
    patchButton->setText("---");
    patchButton->blockSignals(false);
  }
  defPatchH->setEnabled(enPatch);
  defPatchL->setEnabled(enPatch);
  defPatchProg->setEnabled(enPatch);
}

//---------------------------------------------------------
//    setDefaultPatchName
//---------------------------------------------------------

void EditInstrument::setDefaultPatchName(int val)
{
  const char* patchname = getPatchName(val);
  patchButton->blockSignals(true);
  patchButton->setText(QString(patchname));
  patchButton->blockSignals(false);
}

//---------------------------------------------------------
//    getDefaultPatchNumber
//---------------------------------------------------------

int EditInstrument::getDefaultPatchNumber()
{      
  int hval = defPatchH->value() - 1;
  int lval = defPatchL->value() - 1;
  int prog = defPatchProg->value() - 1;
  if(hval == -1)
    hval = 0xff;
  if(lval == -1)
    lval = 0xff;
  if(prog == -1)
    prog = 0xff;
    
  return ((hval & 0xff) << 16) + ((lval & 0xff) << 8) + (prog & 0xff); 
}
      
//---------------------------------------------------------
//    setDefaultPatchNumbers
//---------------------------------------------------------

void EditInstrument::setDefaultPatchNumbers(int val)
{
  int hb;
  int lb;
  int pr;
  
  if(val == CTRL_VAL_UNKNOWN)
    hb = lb = pr = 0;
  else
  {
    hb = ((val >> 16) & 0xff) + 1;
    if (hb == 0x100)
      hb = 0;
    lb = ((val >> 8) & 0xff) + 1;
    if (lb == 0x100)
      lb = 0;
    pr = (val & 0xff) + 1;
    if (pr == 0x100)
      pr = 0;
  } 
    
  defPatchH->blockSignals(true);
  defPatchL->blockSignals(true);
  defPatchProg->blockSignals(true);
  defPatchH->setValue(hb);  
  defPatchL->setValue(lb);  
  defPatchProg->setValue(pr);
  defPatchH->blockSignals(false);
  defPatchL->blockSignals(false);
  defPatchProg->blockSignals(false);
}

//---------------------------------------------------------
//    setDefaultPatchControls
//---------------------------------------------------------

void EditInstrument::setDefaultPatchControls(int val)
{
  setDefaultPatchNumbers(val);
  setDefaultPatchName(val);
}

//---------------------------------------------------------
//   getPatchName
//---------------------------------------------------------

const char* EditInstrument::getPatchName(int prog)
{
      int pr = prog & 0xff;
      if(prog == CTRL_VAL_UNKNOWN || pr == 0xff)
            return "---";
      
      //int hbank = (prog >> 16) & 0x7f;
      //int lbank = (prog >> 8) & 0x7f;
      int hbank = (prog >> 16) & 0xff;
      int lbank = (prog >> 8) & 0xff;

      PatchGroupList* pg = workingInstrument.groups();
      
      for(ciPatchGroup i = pg->begin(); i != pg->end(); ++i) {
            const PatchList& pl = (*i)->patches;
            for (ciPatch ipl = pl.begin(); ipl != pl.end(); ++ipl) {
                  const Patch* mp = *ipl;
                  if (//(mp->typ & tmask) &&
                    (pr == mp->prog)
                    //&& ((drum && mode != MT_GM) || 
                    //   (mp->drum == drumchan))   
                    
                    //&& (hbank == mp->hbank || !hb || mp->hbank == -1)
                    //&& (lbank == mp->lbank || !lb || mp->lbank == -1))
                    && (hbank == mp->hbank || mp->hbank == -1)
                    && (lbank == mp->lbank || mp->lbank == -1))
                        return mp->name.latin1();
                  }
            }
      return "---";
}

