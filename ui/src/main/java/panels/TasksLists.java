package panels;

import io.github.andrewauclair.moderndocking.Dockable;
import io.github.andrewauclair.moderndocking.app.Docking;

import javax.swing.*;
import javax.swing.table.DefaultTableModel;
import java.awt.*;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.io.DataOutputStream;

public class TasksLists extends JPanel implements Dockable {

    private final JTable table;
    private final DefaultTableModel tableModel;
    private final String persistentID;
    private final String title;

    public TasksLists(DataOutputStream output, String persistentID, String title) {
        super(new BorderLayout());
        this.persistentID = persistentID;
        this.title = title;

        Docking.registerDockable(this);

        // JTree on left for groups/lists, JTable on right for tasks

        // add ability to popout specific lists/groups as a new instance of this panel in the docking framework

        JTree tree = new JTree();

        tableModel = new DefaultTableModel(new Object[]{"ID", "Name"}, 0);
        table = new JTable(tableModel);

        JPopupMenu contextMenu = new JPopupMenu();
        JMenuItem start = new JMenuItem("Start");
        start.addActionListener(e -> {
            int selectedRow = table.getSelectedRow();

            if (selectedRow == -1) {
                return;
            }
        });
        contextMenu.add(start);

        table.addMouseListener(new MouseAdapter() {
            @Override
            public void mouseClicked(MouseEvent e) {
                if (SwingUtilities.isRightMouseButton(e)) {
                    contextMenu.show(table, e.getX(), e.getY());
                }
            }
        });

        add(new JScrollPane(table));
    }

    @Override
    public String getPersistentID() {
        return persistentID;
    }

    @Override
    public String getTabText() {
        return title;
    }

    @Override
    public boolean isWrappableInScrollpane() {
        return false;
    }

    public void addTask(int id, String name) {
        tableModel.addRow(new Object[] { id, name });
    }

    public void taskModelUpdated() {

    }
}
