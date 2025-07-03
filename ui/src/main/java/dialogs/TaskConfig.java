package dialogs;

import data.Task;
import taskglacier.MainFrame;

import javax.swing.*;
import javax.swing.table.DefaultTableModel;
import java.awt.*;
import java.io.IOException;
import java.util.Vector;

public class TaskConfig extends JDialog {
    class General extends JPanel {
        General() {
            add(new JLabel(getClass().toGenericString()));
        }
    }

    class Labels extends JPanel {
        Labels() {
            add(new JLabel(getClass().toGenericString()));
        }
    }

    class Sessions extends JPanel {
        Sessions() {
            add(new JLabel(getClass().toGenericString()));
        }
    }

    class TimeEntry extends JPanel {
        TimeEntry() {
            add(new JLabel(getClass().toGenericString()));
        }
    }

    public TaskConfig(MainFrame mainFrame, Task task) {
        setLayout(new GridBagLayout());

//        setTitle("Task Config - " + task.name);

        // general (id, name, status, parent, bugzilla)
        // labels
        // sessions
        // time entry (configure time category and code)

        DefaultTableModel model = new DefaultTableModel();
        JTable list = new JTable(model);

        model.addRow(new Object[]{"General"});
        model.addRow(new Object[]{"Labels"});
        model.addRow(new Object[]{"Sessions"});
        model.addRow(new Object[]{"Time Entry"});

        list.setTableHeader(null);


        JPanel foo = new JPanel();
        JSplitPane split = new JSplitPane();
        split.setLeftComponent(list);

        CardLayout layout = new CardLayout();
        JPanel stack = new JPanel(layout);
        stack.add(new JPanel(), "");
        stack.add(new General(), "General");
        stack.add(new Labels(), "Labels");
        stack.add(new Sessions(), "Sessions");
        stack.add(new TimeEntry(), "Time Entry");
        split.setRightComponent(stack);

        list.getSelectionModel().addListSelectionListener(e -> {
            String name = e.getFirstIndex() != -1 ? (String) model.getValueAt(e.getFirstIndex(), 0) : "";

            layout.show(stack, name);
        });

        foo.add(split);
        
        add(foo);

        // center on the main frame
//        setLocationRelativeTo(mainFrame);
    }

    public static void main(String[] args) throws IOException {
        TaskConfig config = new TaskConfig(null, null);
        config.setVisible(true);
    }
}
