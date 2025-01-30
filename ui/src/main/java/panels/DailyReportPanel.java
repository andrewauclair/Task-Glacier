package panels;

import io.github.andrewauclair.moderndocking.Dockable;

import javax.swing.*;

public class DailyReportPanel extends JPanel implements Dockable {
    @Override
    public String getPersistentID() {
        return "daily-report";
    }

    @Override
    public String getTabText() {
        return "Daily Report";
    }
}
