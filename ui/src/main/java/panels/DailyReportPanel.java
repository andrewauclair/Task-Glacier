package panels;

import io.github.andrewauclair.moderndocking.Dockable;

import javax.swing.*;

public class DailyReportPanel extends JPanel implements Dockable {


    
    public DailyReportPanel() {

    }

    @Override
    public String getPersistentID() {
        return "daily-report";
    }

    @Override
    public String getTabText() {
        return "Daily Report";
    }
}
