package dialogs;

import panels.TaskSearch;
import taskglacier.MainFrame;

import javax.swing.*;
import java.awt.*;

public class SearchDialog extends JDialog {
    public SearchDialog(MainFrame mainFrame) {
        TaskSearch search = new TaskSearch(mainFrame, this, true);

        setModal(true);

        setLayout(new GridBagLayout());
        setTitle("Task Search");

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.weightx = 1;
        gbc.weighty = 1;
        gbc.fill = GridBagConstraints.BOTH;

        add(search, gbc);

        setSize(400, 600);

        // center on the main frame
        setLocationRelativeTo(mainFrame);
    }
}
