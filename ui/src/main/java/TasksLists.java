import ModernDocking.Dockable;
import ModernDocking.Docking;
import org.json.JSONObject;

import javax.swing.*;
import javax.swing.table.DefaultTableModel;
import java.awt.*;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.io.DataOutputStream;
import java.io.IOException;

public class TasksLists extends JPanel implements Dockable {

    private final JTable table;
    private final DefaultTableModel tableModel;

    public TasksLists(DataOutputStream output) {
        super(new GridBagLayout());

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

            JSONObject startTask = new JSONObject();
            startTask.put("command", 3);
            startTask.put("id", tableModel.getValueAt(table.convertRowIndexToModel(selectedRow), 0));

            try {
                MainFrame.sendJSON(output, startTask);
            } catch (IOException ex) {
                throw new RuntimeException(ex);
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
        JSplitPane split = new JSplitPane();
        split.setLeftComponent(new JScrollPane(tree));
        split.setRightComponent(new JScrollPane(table));

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.weightx = 1;
        gbc.weighty = 1;
        gbc.fill = GridBagConstraints.BOTH;

        add(split, gbc);
    }

    @Override
    public String getPersistentID() {
        return "tasks";
    }

    @Override
    public String getTabText() {
        return "Tasks";
    }

    @Override
    public boolean isWrappableInScrollpane() {
        return false;
    }

    public void addTask(int id, String name) {
        tableModel.addRow(new Object[] { id, name });
    }
}
