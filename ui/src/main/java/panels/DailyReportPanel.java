package panels;

import io.github.andrewauclair.moderndocking.Dockable;
import io.github.andrewauclair.moderndocking.app.Docking;
import packets.DailyReportMessage;

import javax.swing.*;
import javax.swing.tree.DefaultMutableTreeNode;
import javax.swing.tree.DefaultTreeModel;
import java.awt.*;

public class DailyReportPanel extends JPanel implements Dockable {
    private DailyReportMessage.DailyReport report = null;

    public DailyReportPanel() {
        Docking.registerDockable(this);

        buildUI();
    }

    private void buildUI() {
        if (report == null) {
            return;
        }

        JLabel date = new JLabel();
        date.setText(String.format("%d/%d/%d", report.month, report.day, report.year));

        setLayout(new GridBagLayout());

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.gridx = 0;
        gbc.gridy = 0;

        add(date, gbc);
        gbc.gridy++;

        JLabel start = new JLabel();
        JLabel end = new JLabel();
        JLabel total = new JLabel();

        add(start, gbc);
        gbc.gridy++;
        add(end, gbc);
        gbc.gridy++;
        add(total, gbc);
        gbc.gridy++;

        JTree times = new JTree();

        DefaultMutableTreeNode root = new DefaultMutableTreeNode();
        DefaultTreeModel model = new DefaultTreeModel(root);
        times.setModel(model);
        
        add(times, gbc);
        gbc.gridy++;

        revalidate();
        repaint();
    }

    public void update(DailyReportMessage message) {
        report = message.getReport();
        buildUI();
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
